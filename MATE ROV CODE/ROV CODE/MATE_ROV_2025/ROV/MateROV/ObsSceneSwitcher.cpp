#include "ObsSceneSwitcher.h"
#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

ObsSceneSwitcher::ObsSceneSwitcher() : resolver(ioc) {}

ObsSceneSwitcher::~ObsSceneSwitcher() {
    if (ws && ws->is_open()) {
        beast::error_code ec;
        ws->close(websocket::close_code::normal, ec);
    }
}

bool ObsSceneSwitcher::connect(const std::string& host, const std::string& port) {
    try {
        ws = std::make_unique<websocket::stream<tcp::socket>>(ioc);
        auto const results = resolver.resolve(host, port);
        net::connect(ws->next_layer(), results);
        ws->handshake(host, "/");

        return handshake() && fetchSceneCollections();
    } catch (const std::exception& e) {
        std::cerr << "[OBS] Connect Error: " << e.what() << "\n";
        return false;
    }
}

bool ObsSceneSwitcher::handshake() {
    beast::flat_buffer buffer;
    ws->read(buffer);
    std::cout << "[OBS] Welcome: " << beast::buffers_to_string(buffer.data()) << "\n";
    buffer.consume(buffer.size());

    json identify = {{"op", 1}, {"d", {{"rpcVersion", 1}}}};
    ws->write(net::buffer(identify.dump()));

    ws->read(buffer);
    std::cout << "[OBS] Identify Response: "
              << beast::buffers_to_string(buffer.data()) << "\n";
    buffer.consume(buffer.size());

    return true;
}

bool ObsSceneSwitcher::fetchSceneCollections() {
    json getCollections = {
        {"op", 6},
        {"d", {{"requestType", "GetSceneCollectionList"}, {"requestId", "get-collections"}}}
    };
    ws->write(net::buffer(getCollections.dump()));

    beast::flat_buffer buffer;
    ws->read(buffer);
    std::string responseStr = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());

    json responseJson = json::parse(responseStr);
    if (responseJson["d"].contains("responseData") &&
        responseJson["d"]["responseData"].contains("sceneCollections")) {
        for (const auto& name : responseJson["d"]["responseData"]["sceneCollections"]) {
            if (name.is_string())
                sceneCollections.push_back(name.get<std::string>());
        }
    }

    return !sceneCollections.empty();
}

bool ObsSceneSwitcher::switchSceneCollection(const std::string& name) {
    if (!ws || !ws->is_open()) return false;

    json msg = {
        {"op", 6},
        {"d", {
            {"requestType", "SetCurrentSceneCollection"},
            {"requestId", "switch-scene"},
            {"requestData", {{"sceneCollectionName", name}}}
        }}
    };

    ws->write(net::buffer(msg.dump()));
    beast::flat_buffer buffer;
    ws->read(buffer);
    std::cout << "[OBS] Switched to: " << name << "\n";
    buffer.consume(buffer.size());
    return true;
}

// Rotate forward through the list
bool ObsSceneSwitcher::switchToNextScene() {
    if (sceneCollections.empty()) return false;
    currentSceneIndex = (currentSceneIndex + 1) % sceneCollections.size();
    return switchSceneCollection(sceneCollections[currentSceneIndex]);
}

// Rotate backwards through the list
bool ObsSceneSwitcher::switchToPreviousScene() {
    if (sceneCollections.empty()) return false;
    currentSceneIndex = (currentSceneIndex - 1 + sceneCollections.size()) % sceneCollections.size();
    return switchSceneCollection(sceneCollections[currentSceneIndex]);
}

// Explicitly pick a specific one by name
bool ObsSceneSwitcher::switchToScene(const std::string& name) {
    auto it = std::find(sceneCollections.begin(), sceneCollections.end(), name);
    if (it == sceneCollections.end()) return false;
    currentSceneIndex = std::distance(sceneCollections.begin(), it);
    return switchSceneCollection(name);
}

bool ObsSceneSwitcher::isConnected() const {
    return ws && ws->is_open();
}
