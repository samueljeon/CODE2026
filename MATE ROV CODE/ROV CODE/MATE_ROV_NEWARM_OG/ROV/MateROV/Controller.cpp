//
// Created by Adam G. on 4/27/25.
//

#include "Controller.h"

Controller::Controller() = default;

Controller::~Controller()
{
    if (gc && SDL_GameControllerGetAttached(gc)) {
        SDL_GameControllerClose(gc);
    }

    gc = nullptr;
}

bool Controller::Init() {
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            gc = SDL_GameControllerOpen(i);
            break;
        }
    }

    if (!gc) {
        return false; // No Controller found
    }

    // Enable motion sensors
    SDL_GameControllerSetSensorEnabled(gc, SDL_SENSOR_GYRO, SDL_TRUE);
    SDL_GameControllerSetSensorEnabled(gc, SDL_SENSOR_ACCEL, SDL_TRUE);

    return true;
}

bool Controller::Init(int deviceIndex, bool gyro, bool accel) {
    if (SDL_IsGameController(deviceIndex)) {
        gc = SDL_GameControllerOpen(deviceIndex);
    }

    if (!gc) {
        return false;
    }

    instID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc));

    if (gyro) {
        SDL_GameControllerSetSensorEnabled(gc, SDL_SENSOR_GYRO, SDL_TRUE);
    }

    if (accel) {
        SDL_GameControllerSetSensorEnabled(gc, SDL_SENSOR_ACCEL, SDL_TRUE);
    }

    return true;
}

int Controller::InstanceID() const {
    return instID;
}

bool Controller::IsAttached() const {
    return gc && SDL_GameControllerGetAttached(gc);
}

bool Controller::hasController() const {
    return gc != nullptr;
}

void Controller::setKMFallback(bool state) {
    KMFallback = state;
}

void Controller::Update() {
    if (!gc) {
        return;
    }

    SDL_GameControllerGetSensorData(gc, SDL_SENSOR_GYRO, gyro.data(), 3);
    SDL_GameControllerGetSensorData(gc, SDL_SENSOR_ACCEL, accel.data(), 3);
}

void Controller::PollEvent(SDL_Event *e) {
    if (KMFallback && !gc) {
        this->e = e;

        if (e->type == SDL_MOUSEMOTION) {
            mme = e->motion;
        }
    }
}

// --------- Buttons ---------
bool Controller::Cross() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A); }
bool Controller::Circle() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B); }
bool Controller::Square() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_X); }
bool Controller::Triangle() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_Y); }
bool Controller::L1() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_LEFTSHOULDER); }
bool Controller::R1() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER); }
bool Controller::L3() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_LEFTSTICK); }
bool Controller::R3() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_RIGHTSTICK); }
bool Controller::Share() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_BACK); }
bool Controller::Options() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_START); }
bool Controller::PSButton() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_GUIDE); }
bool Controller::MicrophoneButton() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_MISC1); }
bool Controller::DpadUp() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_UP); }
bool Controller::DpadDown() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN); }
bool Controller::DpadLeft() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT); }
bool Controller::DpadRight() const { return SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT); }

// --------- Sticks ---------
float Controller::LeftX() const {
    if (KMFallback && !hasController() && mme.state) {
        return ApplyDeadzone(2 * (mme.x / 735.f) - 1);
    }

    return ApplyDeadzone(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0f);
}

float Controller::LeftY() const {
    if (KMFallback && !hasController() && mme.state) {
        return ApplyDeadzone(2 * (mme.y / 500.f) - 1);
    }

    return ApplyDeadzone(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0f);
}

float Controller::RightX() const {
    return ApplyDeadzone(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX) / 32767.0f);
}

float Controller::RightY() const {
    return ApplyDeadzone(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.0f);
}


// --------- Triggers ---------
float Controller::L2() const { return SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0f; }
float Controller::R2() const { return SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0f; }

// --------- Sensors ---------
float Controller::GyroPitch() const { return gyro[0]; }
float Controller::GyroYaw() const { return gyro[1]; }
float Controller::GyroRoll() const { return gyro[2]; }
float Controller::AccelX() const { return accel[0]; }
float Controller::AccelY() const { return accel[1]; }
float Controller::AccelZ() const { return accel[2]; }

void Controller::SetLED(Uint8 r, Uint8 g, Uint8 b) {
    if (!gc) {
        return;
    }

    SDL_GameControllerSetLED(gc, r, g, b);
}

// --------- Rumble ---------
void Controller::Rumble(Uint16 lowFreq, Uint16 highFreq, Uint32 durationMs) {
    if (!gc) {
        return;
    }

    SDL_GameControllerRumble(gc, lowFreq, highFreq, durationMs);
}

// --------- Raw ---------
SDL_GameController *Controller::Raw() {
    return gc;
}

SDL_GameController * Controller::Raw() const {
    return gc;
}
