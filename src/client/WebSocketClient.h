#pragma once

// Phase 5: thin wrapper around ix::WebSocket client.
// Connects to the server, sends one message, prints the echo, exits.
// Full CLI menu wired in Phase 6+ once the protocol is locked.

#include <ixwebsocket/IXWebSocket.h>
#include <string>
#include <functional>

class WebSocketClient {
public:
    explicit WebSocketClient(const std::string& url);

    // Connect, send message, wait for one echo reply, then disconnect.
    void sendAndReceive(const std::string& message);

private:
    ix::WebSocket   ws_;
    std::string     url_;
};
