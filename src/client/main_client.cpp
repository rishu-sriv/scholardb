#include "WebSocketClient.h"

static const std::string SERVER_URL = "ws://localhost:8080";

int main() {
    WebSocketClient client(SERVER_URL);
    client.sendAndReceive("hello from C++ client — phase 5 test");
    return 0;
}
