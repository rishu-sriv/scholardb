#include "StudentRepository.h"
#include "../common/CSVHandler.h"

#include <algorithm>
#include <cctype>

// ── startup ────────────────────────────────────────────────────────────────

void StudentRepository::loadFromCSV(const std::string& path) {
    // unique_lock because we are writing to students
    std::unique_lock lock(rw_mutex);
    students = CSVHandler::loadFromFile(path);
}

// ── mutations (unique_lock) ────────────────────────────────────────────────

bool StudentRepository::addStudent(const Student& s) {
    std::unique_lock lock(rw_mutex);

    // Reject duplicate IDs — without this check two records with the same
    // id can coexist, silently breaking findById, update, and delete.
    for (const auto& existing : students) {
        if (existing.id == s.id) return false;
    }

    students.push_back(s);
    return true;
}

bool StudentRepository::updateStudent(const Student& s) {
    std::unique_lock lock(rw_mutex);

    for (auto& existing : students) {
        if (existing.id == s.id) {
            existing = s;   // full-record replace per UPDATE semantics
            return true;
        }
    }
    return false;   // ID not found
}

bool StudentRepository::deleteStudent(int id) {
    std::unique_lock lock(rw_mutex);

    auto it = std::find_if(students.begin(), students.end(),
                           [id](const Student& s) { return s.id == id; });
    if (it == students.end()) return false;

    students.erase(it);
    return true;
}

// ── reads (shared_lock) ───────────────────────────────────────────────────

std::vector<Student> StudentRepository::getAll() const {
    std::shared_lock lock(rw_mutex);
    return students;
}

std::optional<Student> StudentRepository::findById(int id) const {
    std::shared_lock lock(rw_mutex);

    for (const auto& s : students) {
        if (s.id == id) return s;
    }
    return std::nullopt;
}

std::vector<Student> StudentRepository::findByName(const std::string& name) const {
    std::shared_lock lock(rw_mutex);

    // Case-insensitive substring match
    auto toLower = [](std::string str) {
        for (auto& c : str) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return str;
    };
    const std::string needle = toLower(name);

    std::vector<Student> results;
    for (const auto& s : students) {
        if (toLower(s.name).find(needle) != std::string::npos) {
            results.push_back(s);
        }
    }
    return results;
}

// ── sort (shared_lock, returns copy — canonical vector never touched) ──────

std::vector<Student> StudentRepository::sortBy(SortField field) const {
    std::shared_lock lock(rw_mutex);
    auto copy = students;                       // copy while holding shared_lock
    // lock released after this scope so the sort happens on a local copy,
    // not while blocking readers or writers on the shared data.

    switch (field) {
        case SortField::ID:
            std::sort(copy.begin(), copy.end(),
                      [](const Student& a, const Student& b) { return a.id < b.id; });
            break;
        case SortField::NAME:
            std::sort(copy.begin(), copy.end(),
                      [](const Student& a, const Student& b) { return a.name < b.name; });
            break;
        case SortField::AGE:
            std::sort(copy.begin(), copy.end(),
                      [](const Student& a, const Student& b) { return a.age < b.age; });
            break;
        case SortField::GRADE:
            std::sort(copy.begin(), copy.end(),
                      [](const Student& a, const Student& b) { return a.grade < b.grade; });
            break;
    }

    return copy;
}
