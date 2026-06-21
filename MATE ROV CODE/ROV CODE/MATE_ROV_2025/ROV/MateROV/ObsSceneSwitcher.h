#pragma once
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include "json.hpp"
#include <string>
#include <vector>

class ObsSceneSwitcher {
public:
    ObsSceneSwitcher();
    ~ObsSceneSwitcher();

    bool connect(const std::string& host = "127.0.0.1", const std::string& port = "4455");

    // New functions to rotate or select scene collections:
    bool switchToNextScene();
    bool switchToPreviousScene();
    bool switchToScene(const std::string& name);

    bool isConnected() const;

private:
    using tcp = boost::asio::ip::tcp;
    using websocket_stream = boost::beast::websocket::stream<tcp::socket>;
    using json = nlohmann::json;

    boost::asio::io_context ioc;
    tcp::resolver resolver;
    std::unique_ptr<websocket_stream> ws;

    std::vector<std::string> sceneCollections;
    int currentSceneIndex = 0;

    bool switchSceneCollection(const std::string& name);
    bool handshake();
    bool fetchSceneCollections();
};
