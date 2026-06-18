#pragma once

// ConnectionManager owns the connected-client list and is the only place
// that knows how to send to one client or all clients.
// WebSocketServer calls these methods — it never touches sockets directly.

#include <ixwebsocket/IXWebSocket.h>
#include <unordered_set>
#include <mutex>
#include <string>

class ConnectionManager {
public:
    // Called on every new WebSocket connection (Open event).
    void addClient(ix::WebSocket* ws);

    // Called on every disconnect (Close event).
    void removeClient(ix::WebSocket* ws);

    // Send message to every connected client (used after mutations).
    void broadcast(const std::string& message);

    // Send message to one specific client (QUERY_RESULT, ERROR).
    void sendTo(ix::WebSocket* ws, const std::string& message);

    size_t clientCount() const;

private:
    std::unordered_set<ix::WebSocket*> clients_;
    mutable std::mutex                 mutex_;
};
