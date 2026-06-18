#include "ConnectionManager.h"
#include "../common/Timer.h"
#include <iostream>

void ConnectionManager::addClient(ix::WebSocket* ws) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.insert(ws);
    std::cout << "[connmgr] client added   — total=" << clients_.size() << "\n";
}

void ConnectionManager::removeClient(ix::WebSocket* ws) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.erase(ws);
    std::cout << "[connmgr] client removed — total=" << clients_.size() << "\n";
}

void ConnectionManager::broadcast(const std::string& message) {
    Timer broadcast_t("ws_broadcast");          // ⑤ timed: whole broadcast loop
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto* ws : clients_) {
        Timer transmit_t("ws_transmit");        // ③ timed: single send() per client
        ws->send(message);
    }
    std::cout << "[connmgr] broadcast to " << clients_.size() << " client(s)\n";
}

void ConnectionManager::sendTo(ix::WebSocket* ws, const std::string& message) {
    ws->send(message);
}

size_t ConnectionManager::clientCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return clients_.size();
}
