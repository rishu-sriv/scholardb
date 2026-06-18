#include "WebSocketClient.h"
#include "../common/Protocol.h"
#include <iostream>
#include <cstdio>

// ── Constructor ───────────────────────────────────────────────────────────

WebSocketClient::WebSocketClient(const std::string& url) : url_(url) {
    ws_.setUrl(url_);

    ws_.setOnMessageCallback(
        [this](const ix::WebSocketMessagePtr& msg) {

            if (msg->type == ix::WebSocketMessageType::Open) {
                std::unique_lock<std::mutex> lock(mtx_);
                connected_ = true;
                cv_.notify_all();
                return;
            }

            if (msg->type == ix::WebSocketMessageType::Close) {
                std::cout << "\n[client] server closed the connection\n";
                return;
            }

            if (msg->type == ix::WebSocketMessageType::Error) {
                std::cerr << "\n[client] error: " << msg->errorInfo.reason << "\n";
                return;
            }

            if (msg->type == ix::WebSocketMessageType::Message) {
                std::unique_lock<std::mutex> lock(mtx_);

                if (expectReply_) {
                    // Main thread is waiting for exactly this message.
                    reply_       = msg->str;
                    replyReady_  = true;
                    expectReply_ = false;
                    cv_.notify_one();
                } else {
                    // Unsolicited broadcast from another client's mutation.
                    // Print immediately — do NOT queue it.
                    lock.unlock();
                    printLiveUpdate(msg->str);
                }
            }
        }
    );
}

// ── Internal helpers ──────────────────────────────────────────────────────

std::string WebSocketClient::sendAndWait(const std::string& msg) {
    {
        // Set the flag BEFORE sending so there is no window where the reply
        // can arrive and be mistaken for a live update.
        std::unique_lock<std::mutex> lock(mtx_);
        expectReply_ = true;
        replyReady_  = false;
    }
    ws_.send(msg);

    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this] { return replyReady_; });
    return reply_;
}

std::string WebSocketClient::waitForFirst() {
    // Same as sendAndWait but without sending — used for the initial
    // BROADCAST_UPDATE the server pushes on connect.
    {
        std::unique_lock<std::mutex> lock(mtx_);
        expectReply_ = true;
        replyReady_  = false;
    }
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this] { return replyReady_; });
    return reply_;
}

void WebSocketClient::printStudents(const std::string& raw, const std::string& label) {
    try {
        auto j = nlohmann::json::parse(raw);
        std::string action = j.value("action", "");

        if (action == "ERROR") {
            std::cout << "  ERROR: " << j.value("message", "unknown") << "\n";
            return;
        }

        auto& arr = j["data"];
        if (!label.empty()) std::cout << label << "\n";

        if (!arr.is_array() || arr.empty()) {
            std::cout << "  (no records)\n";
            return;
        }

        std::printf("  %-4s  %-20s  %-4s  %s\n", "ID", "Name", "Age", "Grade");
        std::printf("  %-4s  %-20s  %-4s  %s\n",
                    "----", "--------------------", "----", "-----");
        for (auto& s : arr) {
            std::printf("  %-4d  %-20s  %-4d  %s\n",
                s.value("id",    0),
                s.value("name",  "").c_str(),
                s.value("age",   0),
                s.value("grade", "").c_str());
        }
        std::printf("  %zu record(s)\n", arr.size());
    } catch (...) {
        std::cout << "  (could not parse server response)\n";
    }
}

void WebSocketClient::printLiveUpdate(const std::string& raw) {
    std::cout << "\n[LIVE UPDATE from another client]\n";
    printStudents(raw);
    std::cout << "> " << std::flush;   // re-print the menu prompt
}

// ── run() ─────────────────────────────────────────────────────────────────

void WebSocketClient::run() {
    ws_.start();

    // Wait for WebSocket handshake to complete
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return connected_; });
    }
    std::cout << "[client] connected to " << url_ << "\n";

    // Capture the initial BROADCAST_UPDATE the server sends on connect
    printStudents(waitForFirst(), "[client] current dataset:");

    // ── Menu loop ─────────────────────────────────────────────────────────
    while (true) {
        std::cout << "\n1)List  2)Search  3)Add  4)Update  5)Delete  6)Sort  0)Quit\n> ";

        int choice;
        if (!(std::cin >> choice)) break;
        std::cin.ignore(1000, '\n');

        if (choice == 0) break;

        if (choice == 1) {
            auto reply = sendAndWait(
                nlohmann::json{{"action","LIST"},{"data",nlohmann::json::object()}}.dump());
            printStudents(reply);

        } else if (choice == 2) {
            std::cout << "Name (substring): ";
            std::string q; std::getline(std::cin, q);
            auto reply = sendAndWait(
                nlohmann::json{{"action","SEARCH"},{"data",{{"query",q}}}}.dump());
            printStudents(reply);

        } else if (choice == 3) {
            Student s;
            std::cout << "ID: ";    std::cin >> s.id;
            std::cin.ignore(1000, '\n');
            std::cout << "Name: ";  std::getline(std::cin, s.name);
            std::cout << "Age: ";   std::cin >> s.age;
            std::cin.ignore(1000, '\n');
            std::cout << "Grade: "; std::getline(std::cin, s.grade);
            auto reply = sendAndWait(
                nlohmann::json{{"action","CREATE"},{"data",s.toJson()}}.dump());
            printStudents(reply);

        } else if (choice == 4) {
            Student s;
            std::cout << "ID to update: "; std::cin >> s.id;
            std::cin.ignore(1000, '\n');
            std::cout << "New name: ";  std::getline(std::cin, s.name);
            std::cout << "New age: ";   std::cin >> s.age;
            std::cin.ignore(1000, '\n');
            std::cout << "New grade: "; std::getline(std::cin, s.grade);
            auto reply = sendAndWait(
                nlohmann::json{{"action","UPDATE"},{"data",s.toJson()}}.dump());
            printStudents(reply);

        } else if (choice == 5) {
            int id;
            std::cout << "ID to delete: "; std::cin >> id;
            std::cin.ignore(1000, '\n');
            auto reply = sendAndWait(
                nlohmann::json{{"action","DELETE"},{"data",{{"id",id}}}}.dump());
            printStudents(reply);

        } else if (choice == 6) {
            std::cout << "Sort by — 1)id  2)name  3)age  4)grade\n> ";
            int f; std::cin >> f; std::cin.ignore(1000, '\n');
            std::string field = "id";
            if      (f == 2) field = "name";
            else if (f == 3) field = "age";
            else if (f == 4) field = "grade";
            auto reply = sendAndWait(
                nlohmann::json{{"action","SORT"},{"data",{{"field",field}}}}.dump());
            printStudents(reply);

        } else {
            std::cout << "Unknown option.\n";
        }
    }

    ws_.stop();
    std::cout << "[client] goodbye\n";
}
