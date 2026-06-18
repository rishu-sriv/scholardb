#include "WebSocketServer.h"
#include "../common/Protocol.h"
#include "../common/StudentValidator.h"
#include "../common/CSVHandler.h"
#include "../common/Logger.h"

static const std::string TAG = "server";

// ── Constructor: seed repo, register callback ─────────────────────────────

WebSocketServer::WebSocketServer(int port, const std::string& csvPath)
    : server_(port), port_(port), csvPath_(csvPath)
{
    repo_.loadFromCSV(csvPath_);
    Logger::info(TAG, "loaded " + std::to_string(repo_.getAll().size())
                      + " student(s) from " + csvPath_);

    server_.setOnClientMessageCallback(
        [this](std::shared_ptr<ix::ConnectionState> state,
               ix::WebSocket&                       ws,
               const ix::WebSocketMessagePtr&       msg)
        {
            if (msg->type == ix::WebSocketMessageType::Open) {
                connManager_.addClient(&ws);
                connManager_.sendTo(&ws, makeBroadcastUpdate(repo_.getAll()));
                Logger::info(TAG, "client connected    id=" + state->getId());

            } else if (msg->type == ix::WebSocketMessageType::Close) {
                connManager_.removeClient(&ws);
                Logger::info(TAG, "client disconnected id=" + state->getId());

            } else if (msg->type == ix::WebSocketMessageType::Message) {
                handleMessage(ws, msg->str);

            } else if (msg->type == ix::WebSocketMessageType::Error) {
                Logger::error(TAG, "socket error: " + msg->errorInfo.reason);
            }
        }
    );
}

// ── Message handler: the section 4.4 pipeline ────────────────────────────

void WebSocketServer::handleMessage(ix::WebSocket& ws, const std::string& raw) {
    Logger::info(TAG, "recv: " + raw);

    // ── Stage 1: parse JSON ───────────────────────────────────────────────
    auto parsed = ClientMessage::parse(raw);
    if (!parsed) {
        const std::string err = "Malformed JSON or missing 'action' field";
        Logger::error(TAG, "ERROR sent: " + err);
        connManager_.sendTo(&ws, makeError(err));
        return;
    }

    const Action action = parsed->action;

    // ── Stage 2: validate (CREATE and UPDATE require full student object) ─
    Student s;
    if (action == Action::CREATE || action == Action::UPDATE) {
        try {
            s = Student::fromJson(parsed->data);
        } catch (const std::exception& e) {
            const std::string err = std::string("Invalid payload: ") + e.what();
            Logger::error(TAG, "ERROR sent: " + err);
            connManager_.sendTo(&ws, makeError(err));
            return;
        }
        auto vr = StudentValidator::validate(s);
        if (!vr.valid) {
            Logger::error(TAG, "validation failed: " + vr.error);
            connManager_.sendTo(&ws, makeError(vr.error));
            return;
        }
    }

    // ── Stage 3 + 4: repository call, then persist + broadcast or reply ───
    switch (action) {

        case Action::CREATE: {
            if (!repo_.addStudent(s)) {
                const std::string err = "Student ID already exists";
                Logger::error(TAG, "ERROR sent: " + err);
                connManager_.sendTo(&ws, makeError(err));
                return;
            }
            auto snapshot = repo_.getAll();
            CSVHandler::saveToFile(csvPath_, snapshot);
            connManager_.broadcast(makeBroadcastUpdate(snapshot));
            Logger::info(TAG, "CREATE ok id=" + std::to_string(s.id));
            break;
        }

        case Action::UPDATE: {
            if (!repo_.updateStudent(s)) {
                const std::string err = "Student not found";
                Logger::error(TAG, "ERROR sent: " + err);
                connManager_.sendTo(&ws, makeError(err));
                return;
            }
            auto snapshot = repo_.getAll();
            CSVHandler::saveToFile(csvPath_, snapshot);
            connManager_.broadcast(makeBroadcastUpdate(snapshot));
            Logger::info(TAG, "UPDATE ok id=" + std::to_string(s.id));
            break;
        }

        case Action::DELETE: {
            int id;
            try {
                id = parsed->data.at("id").get<int>();
            } catch (...) {
                const std::string err = "DELETE requires {\"id\": <int>}";
                Logger::error(TAG, "ERROR sent: " + err);
                connManager_.sendTo(&ws, makeError(err));
                return;
            }
            if (!repo_.deleteStudent(id)) {
                const std::string err = "Student not found";
                Logger::error(TAG, "ERROR sent: " + err);
                connManager_.sendTo(&ws, makeError(err));
                return;
            }
            auto snapshot = repo_.getAll();
            CSVHandler::saveToFile(csvPath_, snapshot);
            connManager_.broadcast(makeBroadcastUpdate(snapshot));
            Logger::info(TAG, "DELETE ok id=" + std::to_string(id));
            break;
        }

        case Action::LIST: {
            Logger::info(TAG, "LIST — query result sent to sender");
            connManager_.sendTo(&ws, makeQueryResult(repo_.getAll()));
            break;
        }

        case Action::SEARCH: {
            std::string query;
            try {
                query = parsed->data.at("query").get<std::string>();
            } catch (...) {
                const std::string err = "SEARCH requires {\"query\": <string>}";
                Logger::error(TAG, "ERROR sent: " + err);
                connManager_.sendTo(&ws, makeError(err));
                return;
            }
            auto results = repo_.findByName(query);
            Logger::info(TAG, "SEARCH \"" + query + "\" -> " + std::to_string(results.size()) + " result(s)");
            connManager_.sendTo(&ws, makeQueryResult(results));
            break;
        }

        case Action::SORT: {
            std::string field;
            try {
                field = parsed->data.at("field").get<std::string>();
            } catch (...) {
                const std::string err = "SORT requires {\"field\": \"id\"|\"name\"|\"age\"|\"grade\"}";
                Logger::error(TAG, "ERROR sent: " + err);
                connManager_.sendTo(&ws, makeError(err));
                return;
            }
            SortField sf = SortField::ID;
            if      (field == "name")  sf = SortField::NAME;
            else if (field == "age")   sf = SortField::AGE;
            else if (field == "grade") sf = SortField::GRADE;
            Logger::info(TAG, "SORT by=" + field);
            connManager_.sendTo(&ws, makeQueryResult(repo_.sortBy(sf)));
            break;
        }

        default:
            const std::string err = "Unknown action: " + actionToString(action);
            Logger::error(TAG, "ERROR sent: " + err);
            connManager_.sendTo(&ws, makeError(err));
            break;
    }
}

// ── run ───────────────────────────────────────────────────────────────────

void WebSocketServer::run() {
    bool ok = server_.listenAndStart();
    if (!ok) {
        Logger::error(TAG, "failed to bind on port " + std::to_string(port_));
        return;
    }
    Logger::info(TAG, "running on ws://localhost:" + std::to_string(port_));
    Logger::info(TAG, "press Ctrl-C to stop");
    server_.wait();
}
