#pragma once

// Protocol.h — canonical message shapes for the ScholarDB WebSocket protocol.
//
// Every message on the wire is a JSON object with at least an "action" field.
//
// CLIENT → SERVER
//   { "action": "CREATE", "data": { "id": 101, "name": "Alice", "age": 20, "grade": "A" } }
//   { "action": "UPDATE", "data": { "id": 101, "name": "Alice", "age": 21, "grade": "A+" } }
//   { "action": "DELETE", "data": { "id": 101 } }
//   { "action": "LIST",   "data": {} }
//   { "action": "SEARCH", "data": { "query": "Alice" } }
//   { "action": "SORT",   "data": { "field": "age" } }   // field: id | name | age | grade
//
// SERVER → ALL CLIENTS  (after any successful mutation)
//   { "action": "BROADCAST_UPDATE", "data": [ ...full student array... ] }
//
// SERVER → REQUESTING CLIENT ONLY  (read-only query replies)
//   { "action": "QUERY_RESULT", "data": [ ...filtered/sorted array... ] }
//
// SERVER → REQUESTING CLIENT ONLY  (every rejection path)
//   { "action": "ERROR", "message": "Student ID already exists" }
//   { "action": "ERROR", "message": "Invalid age" }
//   { "action": "ERROR", "message": "Student not found" }
//
// UPDATE semantics: UPDATE always requires a complete Student object —
// id, name, age, and grade must all be present. Partial updates are not
// supported; the full record is replaced. This removes an entire category
// of edge cases (what to do when a field is omitted).

#include <string>
#include <vector>
#include <optional>
#include "Student.h"
#include "../../third_party/json.hpp"

// ── Action enum ──────────────────────────────────────────────────────────────

enum class Action {
    // Client → Server
    CREATE,
    UPDATE,
    DELETE,
    LIST,
    SEARCH,
    SORT,
    // Server → All clients
    BROADCAST_UPDATE,
    // Server → Requesting client only
    QUERY_RESULT,
    ERROR,
    // Sentinel for unknown / parse failure
    UNKNOWN
};

inline std::string actionToString(Action a) {
    switch (a) {
        case Action::CREATE:           return "CREATE";
        case Action::UPDATE:           return "UPDATE";
        case Action::DELETE:           return "DELETE";
        case Action::LIST:             return "LIST";
        case Action::SEARCH:           return "SEARCH";
        case Action::SORT:             return "SORT";
        case Action::BROADCAST_UPDATE: return "BROADCAST_UPDATE";
        case Action::QUERY_RESULT:     return "QUERY_RESULT";
        case Action::ERROR:            return "ERROR";
        default:                       return "UNKNOWN";
    }
}

inline Action actionFromString(const std::string& s) {
    if (s == "CREATE")           return Action::CREATE;
    if (s == "UPDATE")           return Action::UPDATE;
    if (s == "DELETE")           return Action::DELETE;
    if (s == "LIST")             return Action::LIST;
    if (s == "SEARCH")           return Action::SEARCH;
    if (s == "SORT")             return Action::SORT;
    if (s == "BROADCAST_UPDATE") return Action::BROADCAST_UPDATE;
    if (s == "QUERY_RESULT")     return Action::QUERY_RESULT;
    if (s == "ERROR")            return Action::ERROR;
    return Action::UNKNOWN;
}

// ── Incoming message (client → server) ───────────────────────────────────────

struct ClientMessage {
    Action             action = Action::UNKNOWN;
    nlohmann::json     data;   // raw data object; callers pull fields they need

    // Parse a raw WebSocket string into a ClientMessage.
    // Returns nullopt on malformed JSON or missing "action" field.
    static std::optional<ClientMessage> parse(const std::string& raw) {
        try {
            auto j = nlohmann::json::parse(raw);
            if (!j.contains("action") || !j["action"].is_string())
                return std::nullopt;

            ClientMessage msg;
            msg.action = actionFromString(j["action"].get<std::string>());
            msg.data   = j.value("data", nlohmann::json::object());
            return msg;
        } catch (...) {
            return std::nullopt;
        }
    }
};

// ── Outgoing message builders (server → client) ───────────────────────────────
//
// Each builder returns a serialised JSON string ready to pass to
// ws.send() / connectionManager.broadcast() / connectionManager.sendTo().

// Sent to EVERY connected client after a successful CREATE / UPDATE / DELETE.
inline std::string makeBroadcastUpdate(const std::vector<Student>& students) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& s : students) arr.push_back(s.toJson());
    return nlohmann::json{{"action", "BROADCAST_UPDATE"}, {"data", arr}}.dump();
}

// Sent only to the requesting client for LIST / SEARCH / SORT replies.
inline std::string makeQueryResult(const std::vector<Student>& students) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& s : students) arr.push_back(s.toJson());
    return nlohmann::json{{"action", "QUERY_RESULT"}, {"data", arr}}.dump();
}

// Sent only to the requesting client on any failure:
//   - malformed JSON
//   - StudentValidator rejection (bad id / name / age / grade)
//   - StudentRepository rejection (duplicate ID on CREATE, missing ID on UPDATE/DELETE)
inline std::string makeError(const std::string& message) {
    return nlohmann::json{{"action", "ERROR"}, {"message", message}}.dump();
}
