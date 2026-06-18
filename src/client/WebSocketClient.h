#pragma once

#include <ixwebsocket/IXWebSocket.h>
#include <string>
#include <mutex>
#include <condition_variable>

// Threading model:
//   ix callback thread  — receives messages, either stores them as a reply
//                         or prints them immediately as a live update.
//   main thread         — sets expectReply_ = true (under lock) BEFORE
//                         sending a command, then blocks on waitForReply().
//
// This prevents the stale-inbox bug: an unsolicited broadcast that arrives
// while the user is at the menu prompt is printed right away instead of
// being queued and later consumed as the reply to the next operation.
class WebSocketClient {
public:
    explicit WebSocketClient(const std::string& url);
    void run();

private:
    // Set expectReply_ = true, send the message, block until reply arrives.
    std::string sendAndWait(const std::string& msg);

    // Wait for the very first message after connect (initial BROADCAST_UPDATE)
    // without sending anything.
    std::string waitForFirst();

    void printStudents(const std::string& raw, const std::string& label = "");
    void printLiveUpdate(const std::string& raw);

    ix::WebSocket ws_;
    std::string   url_;

    std::mutex              mtx_;
    std::condition_variable cv_;
    bool                    connected_   = false;
    bool                    expectReply_ = false;
    bool                    replyReady_  = false;
    std::string             reply_;
};
