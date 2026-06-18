#pragma once

#include <ixwebsocket/IXWebSocketServer.h>
#include "ConnectionManager.h"
#include "../repository/StudentRepository.h"
#include <string>

class WebSocketServer {
public:
    // csvPath: the file the server loads from at startup and saves to after
    // every successful mutation.
    WebSocketServer(int port, const std::string& csvPath);

    // Blocks until the server is stopped (Ctrl-C).
    void run();

private:
    // Section 4.4 pipeline — four stages per incoming message:
    //   1. parse JSON          → ERROR to sender, stop
    //   2. validate payload    → ERROR to sender, stop   (CREATE/UPDATE only)
    //   3. call repository
    //   4a. mutation ok        → getAll() ONCE → saveToFile + broadcast
    //   4b. mutation failed    → ERROR to sender
    //   4c. read-only query    → QUERY_RESULT to sender, no CSV write
    void handleMessage(ix::WebSocket& ws, const std::string& raw);

    ix::WebSocketServer server_;
    ConnectionManager   connManager_;
    StudentRepository   repo_;
    std::string         csvPath_;
    int                 port_;
};
