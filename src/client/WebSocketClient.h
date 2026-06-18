#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>

// WebSocketClient wraps ix::WebSocket and provides a blocking run() loop.
//
// Threading model:
//   - ix callback thread: pushes received messages into inbox_
//   - main thread:        calls waitForMessage() which blocks until inbox_
//                         has something, then processes and re-shows menu
class WebSocketClient {
public:
    explicit WebSocketClient(const std::string& url);

    // Connects to the server, waits for the initial BROADCAST_UPDATE,
    // then runs the interactive CLI menu until the user quits.
    void run();

private:
    // Block until the server sends a message, return it.
    std::string waitForMessage();

    // Send a pre-built JSON string to the server.
    void send(const std::string& msg);

    // Display a student array from a BROADCAST_UPDATE or QUERY_RESULT.
    void printStudents(const std::string& raw);

    ix::WebSocket ws_;
    std::string   url_;

    std::mutex              mtx_;
    std::condition_variable cv_;
    std::queue<std::string> inbox_;
    bool                    connected_ = false;
};
