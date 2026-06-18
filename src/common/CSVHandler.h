#pragma once

#include <string>
#include <vector>
#include "Student.h"

class CSVHandler {
public:
    // Parse CSV from path; skips header row and logs/skips malformed lines.
    static std::vector<Student> loadFromFile(const std::string& path);

    // Overwrite the file at path with current students (header included).
    static void saveToFile(const std::string& path, const std::vector<Student>& students);
};
