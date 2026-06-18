#include "WebSocketServer.h"

static constexpr int PORT = 8080;

int main() {
    WebSocketServer server(PORT);
    server.run();
    return 0;
}
