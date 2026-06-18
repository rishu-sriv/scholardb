#include "WebSocketServer.h"

static constexpr int        PORT     = 8080;
static const    std::string CSV_PATH = "students.csv";

int main() {
    WebSocketServer server(PORT, CSV_PATH);
    server.run();
    return 0;
}
