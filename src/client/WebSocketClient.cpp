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

            } else if (msg->type == ix::WebSocketMessageType::Message) {
                // Push every incoming message into the inbox for the main
                // thread to consume via waitForMessage().
                std::unique_lock<std::mutex> lock(mtx_);
                inbox_.push(msg->str);
                cv_.notify_one();

            } else if (msg->type == ix::WebSocketMessageType::Error) {
                std::cerr << "[client] error: " << msg->errorInfo.reason << "\n";
                // Unblock any waiting main thread
                std::unique_lock<std::mutex> lock(mtx_);
                inbox_.push(makeError(msg->errorInfo.reason));
                cv_.notify_one();

            } else if (msg->type == ix::WebSocketMessageType::Close) {
                std::cout << "[client] disconnected from server\n";
            }
        }
    );
}

// ── Internal helpers ──────────────────────────────────────────────────────

std::string WebSocketClient::waitForMessage() {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this] { return !inbox_.empty(); });
    std::string msg = inbox_.front();
    inbox_.pop();
    return msg;
}

void WebSocketClient::send(const std::string& msg) {
    ws_.send(msg);
}

void WebSocketClient::printStudents(const std::string& raw) {
    try {
        auto j = nlohmann::json::parse(raw);
        std::string action = j.value("action", "");

        if (action == "ERROR") {
            std::cout << "  ERROR: " << j.value("message", "unknown") << "\n";
            return;
        }

        auto& arr = j["data"];
        if (!arr.is_array() || arr.empty()) {
            std::cout << "  (no records)\n";
            return;
        }

        std::printf("\n  %-4s  %-20s  %-4s  %s\n", "ID", "Name", "Age", "Grade");
        std::printf("  %-4s  %-20s  %-4s  %s\n", "----", "--------------------", "----", "-----");
        for (auto& s : arr) {
            std::printf("  %-4d  %-20s  %-4d  %s\n",
                s.value("id", 0),
                s.value("name", "").c_str(),
                s.value("age", 0),
                s.value("grade", "").c_str());
        }
        std::printf("  %zu record(s)\n", arr.size());
    } catch (...) {
        std::cout << "  (could not parse server response)\n";
    }
}

// ── run() — connect, wait for initial broadcast, then menu loop ───────────

void WebSocketClient::run() {
    ws_.start();

    // Wait for the WebSocket connection to open
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return connected_; });
    }
    std::cout << "[client] connected to " << url_ << "\n";

    // The server immediately sends a BROADCAST_UPDATE on connect —
    // display it as the starting dataset.
    std::cout << "[client] current dataset:\n";
    printStudents(waitForMessage());

    // ── Menu loop ─────────────────────────────────────────────────────────
    while (true) {
        std::cout << "\n1)List  2)Search  3)Add  4)Update  5)Delete  6)Sort  0)Quit\n> ";

        int choice;
        if (!(std::cin >> choice)) break;
        std::cin.ignore(1000, '\n');

        if (choice == 0) break;

        if (choice == 1) {
            // LIST — QUERY_RESULT comes back to sender only
            send(nlohmann::json{{"action","LIST"},{"data",nlohmann::json::object()}}.dump());
            printStudents(waitForMessage());

        } else if (choice == 2) {
            // SEARCH
            std::cout << "Name (substring): ";
            std::string q; std::getline(std::cin, q);
            send(nlohmann::json{{"action","SEARCH"},{"data",{{"query",q}}}}.dump());
            printStudents(waitForMessage());

        } else if (choice == 3) {
            // CREATE
            Student s;
            std::cout << "ID: ";    std::cin >> s.id;
            std::cin.ignore(1000, '\n');
            std::cout << "Name: ";  std::getline(std::cin, s.name);
            std::cout << "Age: ";   std::cin >> s.age;
            std::cin.ignore(1000, '\n');
            std::cout << "Grade: "; std::getline(std::cin, s.grade);
            send(nlohmann::json{{"action","CREATE"},{"data",s.toJson()}}.dump());
            // Server broadcasts to ALL on success, or sends ERROR to us on failure
            printStudents(waitForMessage());

        } else if (choice == 4) {
            // UPDATE — send full record (no partial updates per protocol)
            Student s;
            std::cout << "ID to update: "; std::cin >> s.id;
            std::cin.ignore(1000, '\n');
            std::cout << "New name: ";  std::getline(std::cin, s.name);
            std::cout << "New age: ";   std::cin >> s.age;
            std::cin.ignore(1000, '\n');
            std::cout << "New grade: "; std::getline(std::cin, s.grade);
            send(nlohmann::json{{"action","UPDATE"},{"data",s.toJson()}}.dump());
            printStudents(waitForMessage());

        } else if (choice == 5) {
            // DELETE
            int id;
            std::cout << "ID to delete: "; std::cin >> id;
            std::cin.ignore(1000, '\n');
            send(nlohmann::json{{"action","DELETE"},{"data",{{"id",id}}}}.dump());
            printStudents(waitForMessage());

        } else if (choice == 6) {
            // SORT
            std::cout << "Sort by — 1)id  2)name  3)age  4)grade\n> ";
            int f; std::cin >> f; std::cin.ignore(1000, '\n');
            std::string field = "id";
            if      (f == 2) field = "name";
            else if (f == 3) field = "age";
            else if (f == 4) field = "grade";
            send(nlohmann::json{{"action","SORT"},{"data",{{"field",field}}}}.dump());
            printStudents(waitForMessage());

        } else {
            std::cout << "Unknown option.\n";
        }
    }

    ws_.stop();
    std::cout << "[client] goodbye\n";
}
