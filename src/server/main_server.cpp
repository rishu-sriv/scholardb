#include "WebSocketServer.h"
#include "../common/Timer.h"
#include <csignal>
#include <cstdlib>

static constexpr int        PORT     = 8080;
static const    std::string CSV_PATH = "students.csv";

static void onShutdown(int) {
    PerformanceStats::instance().print();
    std::exit(0);
}

int main() {
    std::signal(SIGINT,  onShutdown);   // Ctrl-C
    std::signal(SIGTERM, onShutdown);   // kill / Docker stop

    WebSocketServer server(PORT, CSV_PATH);
    server.run();
    return 0;
}
