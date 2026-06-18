#include "WebSocketClient.h"
#include <iostream>
#include <mutex>
#include <condition_variable>

WebSocketClient::WebSocketClient(const std::string& url) : url_(url) {
    ws_.setUrl(url_);
}

void WebSocketClient::sendAndReceive(const std::string& message) {
    // Used to block main thread until we get the echo reply back.
    std::mutex              mtx;
    std::condition_variable cv;
    bool                    done = false;

    ws_.setOnMessageCallback(
        [&](const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Open) {
                std::cout << "[client] connected to " << url_ << "\n";
                std::cout << "[client] sending: " << message << "\n";
                ws_.send(message);

            } else if (msg->type == ix::WebSocketMessageType::Message) {
                std::cout << "[client] echo received: " << msg->str << "\n";
                // Signal main thread we're done
                std::unique_lock<std::mutex> lock(mtx);
                done = true;
                cv.notify_one();

            } else if (msg->type == ix::WebSocketMessageType::Error) {
                std::cerr << "[client] error: " << msg->errorInfo.reason << "\n";
                std::unique_lock<std::mutex> lock(mtx);
                done = true;
                cv.notify_one();

            } else if (msg->type == ix::WebSocketMessageType::Close) {
                std::cout << "[client] disconnected\n";
            }
        }
    );

    ws_.start();

    // Block until echo arrives (or error)
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&] { return done; });

    ws_.stop();
}
