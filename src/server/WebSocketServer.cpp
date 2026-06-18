#include "WebSocketServer.h"
#include "../common/Protocol.h"
#include "../common/StudentValidator.h"
#include "../common/CSVHandler.h"
#include <iostream>

// ── Constructor: seed repo, register callback ─────────────────────────────

WebSocketServer::WebSocketServer(int port, const std::string& csvPath)
    : server_(port), port_(port), csvPath_(csvPath)
{
    repo_.loadFromCSV(csvPath_);
    std::cout << "[server] loaded " << repo_.getAll().size()
              << " student(s) from " << csvPath_ << "\n";

    server_.setOnClientMessageCallback(
        [this](std::shared_ptr<ix::ConnectionState> state,
               ix::WebSocket&                       ws,
               const ix::WebSocketMessagePtr&       msg)
        {
            if (msg->type == ix::WebSocketMessageType::Open) {
                connManager_.addClient(&ws);
                // Send the full current dataset to the newly connected client
                // so it isn't staring at an empty table.
                connManager_.sendTo(&ws, makeBroadcastUpdate(repo_.getAll()));
                std::cout << "[server] client connected    id=" << state->getId() << "\n";

            } else if (msg->type == ix::WebSocketMessageType::Close) {
                connManager_.removeClient(&ws);
                std::cout << "[server] client disconnected id=" << state->getId() << "\n";

            } else if (msg->type == ix::WebSocketMessageType::Message) {
                handleMessage(ws, msg->str);

            } else if (msg->type == ix::WebSocketMessageType::Error) {
                std::cerr << "[server] socket error: " << msg->errorInfo.reason << "\n";
            }
        }
    );
}

// ── Message handler: the section 4.4 pipeline ────────────────────────────

void WebSocketServer::handleMessage(ix::WebSocket& ws, const std::string& raw) {
    std::cout << "[server] recv: " << raw << "\n";

    // ── Stage 1: parse JSON ───────────────────────────────────────────────
    auto parsed = ClientMessage::parse(raw);
    if (!parsed) {
        connManager_.sendTo(&ws, makeError("Malformed JSON or missing 'action' field"));
        return;
    }

    const Action action = parsed->action;

    // ── Stage 2: validate (CREATE and UPDATE require full student object) ─
    Student s;
    if (action == Action::CREATE || action == Action::UPDATE) {
        try {
            s = Student::fromJson(parsed->data);
        } catch (const std::exception& e) {
            connManager_.sendTo(&ws,
                makeError(std::string("Invalid payload: ") + e.what()));
            return;
        }
        auto vr = StudentValidator::validate(s);
        if (!vr.valid) {
            connManager_.sendTo(&ws, makeError(vr.error));
            return;
        }
    }

    // ── Stage 3 + 4: repository call, then persist + broadcast or reply ───
    switch (action) {

        // ── Mutations ────────────────────────────────────────────────────

        case Action::CREATE: {
            if (!repo_.addStudent(s)) {
                connManager_.sendTo(&ws, makeError("Student ID already exists"));
                return;
            }
            // Snapshot rule: ONE getAll(), reused for both save and broadcast.
            auto snapshot = repo_.getAll();
            CSVHandler::saveToFile(csvPath_, snapshot);
            connManager_.broadcast(makeBroadcastUpdate(snapshot));
            std::cout << "[server] CREATE ok id=" << s.id << "\n";
            break;
        }

        case Action::UPDATE: {
            if (!repo_.updateStudent(s)) {
                connManager_.sendTo(&ws, makeError("Student not found"));
                return;
            }
            auto snapshot = repo_.getAll();
            CSVHandler::saveToFile(csvPath_, snapshot);
            connManager_.broadcast(makeBroadcastUpdate(snapshot));
            std::cout << "[server] UPDATE ok id=" << s.id << "\n";
            break;
        }

        case Action::DELETE: {
            int id;
            try {
                id = parsed->data.at("id").get<int>();
            } catch (...) {
                connManager_.sendTo(&ws, makeError("DELETE requires {\"id\": <int>}"));
                return;
            }
            if (!repo_.deleteStudent(id)) {
                connManager_.sendTo(&ws, makeError("Student not found"));
                return;
            }
            auto snapshot = repo_.getAll();
            CSVHandler::saveToFile(csvPath_, snapshot);
            connManager_.broadcast(makeBroadcastUpdate(snapshot));
            std::cout << "[server] DELETE ok id=" << id << "\n";
            break;
        }

        // ── Read-only queries: reply to sender only, no CSV write ────────

        case Action::LIST: {
            connManager_.sendTo(&ws, makeQueryResult(repo_.getAll()));
            break;
        }

        case Action::SEARCH: {
            std::string query;
            try {
                query = parsed->data.at("query").get<std::string>();
            } catch (...) {
                connManager_.sendTo(&ws, makeError("SEARCH requires {\"query\": <string>}"));
                return;
            }
            connManager_.sendTo(&ws, makeQueryResult(repo_.findByName(query)));
            break;
        }

        case Action::SORT: {
            std::string field;
            try {
                field = parsed->data.at("field").get<std::string>();
            } catch (...) {
                connManager_.sendTo(&ws, makeError("SORT requires {\"field\": \"id\"|\"name\"|\"age\"|\"grade\"}"));
                return;
            }
            SortField sf = SortField::ID;
            if      (field == "name")  sf = SortField::NAME;
            else if (field == "age")   sf = SortField::AGE;
            else if (field == "grade") sf = SortField::GRADE;
            connManager_.sendTo(&ws, makeQueryResult(repo_.sortBy(sf)));
            break;
        }

        default:
            connManager_.sendTo(&ws, makeError("Unknown action: " + actionToString(action)));
            break;
    }
}

// ── run ───────────────────────────────────────────────────────────────────

void WebSocketServer::run() {
    bool ok = server_.listenAndStart();
    if (!ok) {
        std::cerr << "[server] failed to bind on port " << port_ << "\n";
        return;
    }
    std::cout << "[server] running on ws://localhost:" << port_ << "\n";
    std::cout << "[server] press Ctrl-C to stop\n";
    server_.wait();
}
