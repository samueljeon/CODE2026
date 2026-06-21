#include <boost/asio.hpp>
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <iostream>
#include <chrono>
#include "Visual.h"
#include "Controller.h"
#include "CalibManager.h"
#include "ObsSceneSwitcher.h"
// #include "CameraManager.h"

using namespace boost;
#include <boost/system/error_code.hpp>


#pragma pack(push, 1)
struct ROVPacket {
    uint8_t start = 0xAA;
    float tleft = 0;
    float tright = 0;
    float tup_left = 0;
    float tup_right = 0;
    float rotate = 0.5;        // Elbow servo:    L2/R2 triggers
    float dc_motor = 0.0;      // DC motor 1:     Dpad Left/Right
    float dc_motor2 = 0.0;     // DC motor 2:     Dpad Up/Down
    uint8_t end = 0x55;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct TelemetryPacket {
    uint8_t start = 0xAB;
    float temperature;
    float humidity;
    uint8_t end = 0x54;
};
#pragma pack(pop)


asio::serial_port *serial = nullptr;

std::string serialPortPath = "/dev/tty.usbmodem1201";
const int baudRate = 115200;

TelemetryPacket incoming;

void asyncReadTelemetry(asio::serial_port& serial, asio::io_context& io_context);

void handleTelemetryRead(const boost::system::error_code& ec, std::size_t bytes_transferred,
                         uint8_t* buffer, asio::serial_port& serial, asio::io_context& io_context) {
    if (!ec && bytes_transferred == sizeof(TelemetryPacket)) {
        TelemetryPacket* pkt = reinterpret_cast<TelemetryPacket*>(buffer);
        if (pkt->start == 0xAB && pkt->end == 0x54) {
            incoming = *pkt;
            std::cout << "[Telemetry] Humidity: " << pkt->humidity
                      << " % | Temp: " << pkt->temperature << " °C\n";
        }
    }
    asyncReadTelemetry(serial, io_context);
}

void asyncReadTelemetry(asio::serial_port& serial, asio::io_context& io_context) {
    auto buffer = std::make_shared<std::array<uint8_t, sizeof(TelemetryPacket)>>();

    asio::async_read(serial,
        asio::buffer(*buffer),
        [&serial, &io_context, buffer](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec && bytes_transferred == sizeof(TelemetryPacket)) {
                TelemetryPacket* pkt = reinterpret_cast<TelemetryPacket*>(buffer->data());
                if (pkt->start == 0xAB && pkt->end == 0x54) {
                    incoming = *pkt;
                    std::cout << "[Telemetry] Humidity: " << pkt->humidity
                              << " % | Temp: " << pkt->temperature << " °C\n";
                }
            }
            asyncReadTelemetry(serial, io_context);
        }
    );
}

bool tryConnect(asio::io_context &io) {
    try {
        delete serial;

        serial = new asio::serial_port(io, serialPortPath);
        serial->set_option(asio::serial_port_base::baud_rate(baudRate));
        std::cerr << "[Serial] Connected to " << serialPortPath << "\n";

        asyncReadTelemetry(*serial, io);

        return true;
    } catch (...) {
        std::cerr << "[Serial] Connection failed\n";

        delete serial;
        serial = nullptr;

        return false;
    }
}


unsigned int packetsSent = 0;

void RenderDockSpace(float fps) {
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceID = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);

    if (ImGui::BeginMenuBar()) {
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Separator();
        ImGui::Text("Packets Sent: %d", packetsSent);
        ImGui::EndMenuBar();
    }

    ImGui::End();
}


int main() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

    SDL_Window* window = SDL_CreateWindow("MateROV", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 800, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImFontConfig font_cfg;
    font_cfg.OversampleH = 4;
    font_cfg.OversampleV = 4;
    font_cfg.PixelSnapH = false;

    io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/Menlo.ttc", 18.0f, &font_cfg);

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    ImGui::GetStyle().AntiAliasedLines = true;
    ImGui::GetStyle().AntiAliasedFill  = true;

    std::vector<std::unique_ptr<Controller>> controllers;

    CalibManager calib;
    calib.registerControl("TLeft",      -1.0f, 1.0f, true);
    calib.registerControl("TRight",     -1.0f, 1.0f, true);
    calib.registerControl("TUp Left",   -1.0f, 1.0f, true);
    calib.registerControl("TUp Right",  -1.0f, 1.0f, true);
    calib.registerControl("Rotate",      0.0f, 1.0f);        // Elbow servo
    calib.registerControl("DC Motor",   -1.0f, 1.0f, true);  // Dpad Left/Right
    calib.registerControl("DC Motor 2", -1.0f, 1.0f, true);  // Dpad Up/Down
    calib.refreshProfiles();
    calib.loadLastUsedProfile();

    asio::io_context io_context;
    tryConnect(io_context);

    if (serial && serial->is_open()) {
        asyncReadTelemetry(*serial, io_context);
    }

    bool quit = false;
    SDL_Event e;

    ROVPacket pkt;
    ROVPacket lastSentPacket{};
    auto lastReconnectAttempt = std::chrono::steady_clock::now() - std::chrono::seconds(5);

    ObsSceneSwitcher obs;
    obs.connect();

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);

            if (e.type == SDL_CONTROLLERDEVICEADDED) {
                int idx = e.cdevice.which;
                auto c = std::make_unique<Controller>();
                if (c->Init(idx, true, true)) {
                    std::cout << "[HotPlug] added idx " << idx << '\n';
                    controllers.push_back(std::move(c));
                }
            }

            if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
                int goneID = e.cdevice.which;
                std::erase_if(controllers, [goneID](auto& p){
                    return p && p->InstanceID() == goneID;
                });
                std::cout << "[HotPlug] removed instance " << goneID
                          << "   remaining: " << controllers.size() << '\n';
            }

            for (auto& c : controllers) {
                c->PollEvent(&e);
            }

            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        for (auto it = controllers.begin(); it != controllers.end(); ) {
            if (!(*it)->IsAttached()) {
                std::cout << "[GC] removed detached pad\n";
                it = controllers.erase(it);
            } else {
                ++it;
            }
        }

        while (io_context.poll_one()) {}

        for (auto& c : controllers) {
            c->Update();
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        {
            float fps = ImGui::GetIO().Framerate;
            RenderDockSpace(fps);
        }

        {
            ImGui::Begin("ROV Packet Stats");
            ImGui::Text("(L Stick)      Left Thruster:   %+.2f", pkt.tleft);
            ImGui::Text("(L Stick)      Right Thruster:  %+.2f", pkt.tright);
            ImGui::Text("(R Stick)      Up Thrusters:    %+.2f", pkt.tup_left);
            ImGui::Text("(L2/R2)        Elbow Servo:     %+.2f", pkt.rotate);
            ImGui::Text("(Dpad L/R)     DC Motor 1:      %+.2f", pkt.dc_motor);
            ImGui::Text("(Dpad Up/Down) DC Motor 2:      %+.2f", pkt.dc_motor2);
            ImGui::End();
        }

        auto now = std::chrono::steady_clock::now();
        if (!serial || !serial->is_open()) {
            if (now - lastReconnectAttempt >= std::chrono::seconds(2)) {
                lastReconnectAttempt = now;
                tryConnect(io_context);
            }
        }

        // --- Thrusters ---
        float tleft = 0.0f, tright = 0.0f;
        for (auto& c : controllers) {
            float x = c->LeftX();
            float y = -c->LeftY();
            tleft  += y + x;
            tright += y - x;
        }

        float z = 0.0f;
        for (auto& c : controllers) {
            z += c->RightY();
        }
        pkt.tup_left = pkt.tup_right = std::clamp(z, -1.0f, 1.0f);

        // --- Elbow servo: L2/R2 triggers, accumulated ---
        float rotateDelta = 0.0f;
        for (auto& c : controllers) {
            rotateDelta += std::pow(c->R2(), 1.2f) * 0.005f;  // Extend
            rotateDelta -= std::pow(c->L2(), 1.2f) * 0.005f;  // Retract
        }


        // --- DC Motor 1: Dpad Left/Right, direct ---
        float dcMotor = 0.0f;
        for (auto& c : controllers) {
            if (c->DpadRight()) dcMotor =  1.0f;
            if (c->DpadLeft())  dcMotor = -1.0f;
        }

        // --- DC Motor 2: Dpad Up/Down, direct ---
        float dcMotor2 = 0.0f;
        for (auto& c : controllers) {
            if (c->DpadUp())   dcMotor2 =  1.0f;
            if (c->DpadDown()) dcMotor2 = -1.0f;
        }

        pkt.tleft     = -calib.apply("TLeft",      tleft);
        pkt.tright    = -calib.apply("TRight",     tright);
        pkt.tup_left  =  calib.apply("TUp Left",   z);
        pkt.tup_right =  calib.apply("TUp Right",  z);
        pkt.rotate    =  std::clamp(pkt.rotate + rotateDelta, 0.0f, 1.0f);
        pkt.rotate    =  calib.apply("Rotate",     pkt.rotate);
        pkt.dc_motor  =  calib.apply("DC Motor",   dcMotor);
        pkt.dc_motor2 =  calib.apply("DC Motor 2", dcMotor2);

        if (std::memcmp(&pkt, &lastSentPacket, sizeof(ROVPacket)) != 0) {
            try {
                if (serial && serial->is_open()) {
                    asio::write(*serial, asio::buffer(&pkt, sizeof(pkt)));
                    packetsSent++;
                }
                lastSentPacket = pkt;
            } catch (...) {
                delete serial;
                serial = nullptr;
            }
        }

        for (auto& c : controllers) {
            if (c->L2() > 0.1 || c->R2() > 0.1) {
                c->Rumble(c->L2() * 0xFFFF, c->R2() * 0xFFFF, 100);
            }
        }

        {
            ImGui::Begin("Calibration");
            calib.renderUI();
            ImGui::End();
        }

        static std::vector<bool> lastL1State(controllers.size(), false);
        static std::vector<bool> lastR1State(controllers.size(), false);

        if (lastL1State.size() != controllers.size()) {
            lastL1State.resize(controllers.size(), false);
            lastR1State.resize(controllers.size(), false);
        }

        if (!controllers.empty()) {
            Visual::DrawImGuiController(*controllers[0]);

            for (size_t i = 0; i < controllers.size(); ++i) {
                bool currL1 = controllers[i]->L1();
                bool currR1 = controllers[i]->R1();

                bool freshL1 = currL1 && !lastL1State[i];
                bool freshR1 = currR1 && !lastR1State[i];

                if (freshL1) obs.switchToPreviousScene();
                if (freshR1) obs.switchToNextScene();

                lastL1State[i] = currL1;
                lastR1State[i] = currR1;
            }
        }

        ImGui::Begin("Telemetry");
        ImGui::Text("Temp: %.2f °C", incoming.temperature);
        ImGui::Text("Humidity: %.2f %%", incoming.humidity);
        ImGui::End();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 15, 15, 15, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(15);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (serial && serial->is_open()) serial->close();
    delete serial;
    return 0;
}
