#include "WebSocketClient.h"

static const std::string SERVER_URL = "ws://localhost:8080";

int main() {
    WebSocketClient client(SERVER_URL);
    client.run();
    return 0;
}
