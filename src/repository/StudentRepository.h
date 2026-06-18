#pragma once

#include <vector>
#include <optional>
#include <shared_mutex>
#include <string>
#include "../common/Student.h"

enum class SortField { ID, NAME, AGE, GRADE };

class StudentRepository {
public:
    // Seeds the in-memory vector from disk. Called once at startup.
    // This is the only place the repository touches CSVHandler.
    void loadFromCSV(const std::string& path);

    // Returns false if a student with the same ID already exists.
    bool addStudent(const Student& s);

    std::vector<Student>       getAll()                            const;
    std::optional<Student>     findById(int id)                    const;
    std::vector<Student>       findByName(const std::string& name) const;

    // Full-record replace: UPDATE requires id, name, age, grade all present.
    // Returns false if the ID does not exist.
    bool updateStudent(const Student& s);

    // Returns false if the ID does not exist.
    bool deleteStudent(int id);

    // Returns a sorted *copy* — never mutates the canonical students vector.
    std::vector<Student> sortBy(SortField field) const;

private:
    std::vector<Student>      students;
    mutable std::shared_mutex rw_mutex;
};
