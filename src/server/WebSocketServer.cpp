#include "WebSocketServer.h"
#include "../common/Protocol.h"
#include "../common/StudentValidator.h"
#include <iostream>

WebSocketServer::WebSocketServer(int port)
    : server_(port), port_(port)
{
    server_.setOnClientMessageCallback(
        [this](std::shared_ptr<ix::ConnectionState> state,
               ix::WebSocket&                       ws,
               const ix::WebSocketMessagePtr&       msg)
        {
            if (msg->type == ix::WebSocketMessageType::Open) {
                connManager_.addClient(&ws);
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

void WebSocketServer::handleMessage(ix::WebSocket& ws, const std::string& raw) {
    std::cout << "[server] received: " << raw << "\n";

    // ── Stage 1: parse JSON ────────────────────────────────────────────────
    auto parsed = ClientMessage::parse(raw);
    if (!parsed) {
        connManager_.sendTo(&ws, makeError("Malformed JSON or missing 'action' field"));
        return;
    }

    const Action action = parsed->action;
    std::cout << "[server] action=" << actionToString(action) << "\n";

    // ── Stage 2: validate payload (CREATE and UPDATE only) ────────────────
    if (action == Action::CREATE || action == Action::UPDATE) {
        // The full student object must be present — UPDATE replaces the
        // entire record, so all four fields are always required.
        Student s;
        try {
            s = Student::fromJson(parsed->data);
        } catch (const std::exception& e) {
            connManager_.sendTo(&ws,
                makeError(std::string("Invalid payload: ") + e.what()));
            return;
        }

        auto result = StudentValidator::validate(s);
        if (!result.valid) {
            std::cout << "[server] validation failed: " << result.error << "\n";
            connManager_.sendTo(&ws, makeError(result.error));
            return;     // ← stop here; repository never called with invalid data
        }

        // Validation passed — repository not wired yet (Phase 9)
        std::cout << "[server] validation passed for id=" << s.id
                  << " — repository not yet wired (Phase 9)\n";
        connManager_.sendTo(&ws, makeError("Repository not yet wired — Phase 9"));
        return;
    }

    // ── Other actions: echo raw message back for now ───────────────────────
    // DELETE / LIST / SEARCH / SORT will get real handlers in Phase 9.
    connManager_.sendTo(&ws, raw);
}

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
