#include "CSVHandler.h"
#include "Timer.h"

#include <fstream>
#include <sstream>
#include <iostream>

std::vector<Student> CSVHandler::loadFromFile(const std::string& path) {
    Timer t("csv_load");                // ① timed: CSV load at startup
    std::vector<Student> students;
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "[CSVHandler] Cannot open file: " << path << "\n";
        return students;
    }

    std::string line;
    // Skip header row
    if (!std::getline(file, line)) {
        std::cerr << "[CSVHandler] File is empty: " << path << "\n";
        return students;
    }

    int lineNum = 1;
    while (std::getline(file, line)) {
        ++lineNum;
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string token;
        std::vector<std::string> fields;

        while (std::getline(ss, token, ',')) {
            fields.push_back(token);
        }

        if (fields.size() != 4) {
            std::cerr << "[CSVHandler] Skipping malformed line " << lineNum
                      << ": expected 4 fields, got " << fields.size() << "\n";
            continue;
        }

        try {
            Student s;
            s.id    = std::stoi(fields[0]);
            s.name  = fields[1];
            s.age   = std::stoi(fields[2]);
            s.grade = fields[3];
            students.push_back(s);
        } catch (const std::exception& e) {
            std::cerr << "[CSVHandler] Skipping line " << lineNum
                      << ": parse error — " << e.what() << "\n";
        }
    }

    return students;
}

void CSVHandler::saveToFile(const std::string& path, const std::vector<Student>& students) {
    Timer t("csv_save");                // ④ timed: CSV save after each mutation
    std::ofstream file(path);

    if (!file.is_open()) {
        std::cerr << "[CSVHandler] Cannot write to file: " << path << "\n";
        return;
    }

    file << "id,name,age,grade\n";
    for (const auto& s : students) {
        file << s.id << "," << s.name << "," << s.age << "," << s.grade << "\n";
    }
}
