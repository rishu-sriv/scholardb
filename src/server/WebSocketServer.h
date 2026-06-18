#pragma once

#include <ixwebsocket/IXWebSocketServer.h>
#include "ConnectionManager.h"

class WebSocketServer {
public:
    explicit WebSocketServer(int port);

    // Blocks until the server is stopped (Ctrl-C).
    void run();

private:
    // Four-stage pipeline per incoming message:
    //   1. parse JSON       → malformed: sendTo ERROR, stop
    //   2. validate payload → invalid:   sendTo ERROR, stop  (CREATE/UPDATE only)
    //   3. repository       → not wired yet (Phase 9)
    //   4. persist + broadcast
    void handleMessage(ix::WebSocket& ws, const std::string& raw);

    ix::WebSocketServer server_;
    ConnectionManager   connManager_;
    int                 port_;
};
