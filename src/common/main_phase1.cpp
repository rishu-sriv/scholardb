#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "Student.h"
#include "CSVHandler.h"
#include "../repository/StudentRepository.h"

// ── display helpers ────────────────────────────────────────────────────────

static void printHeader() {
    std::cout << "\nID   Name                Age  Grade\n";
    std::cout << "---- ------------------- ---- -----\n";
}

static void printStudent(const Student& s) {
    std::printf("%-4d %-19s %-4d %s\n",
                s.id, s.name.c_str(), s.age, s.grade.c_str());
}

static void printList(const std::vector<Student>& list) {
    if (list.empty()) { std::cout << "  (no records)\n"; return; }
    printHeader();
    for (const auto& s : list) printStudent(s);
    std::cout << "  " << list.size() << " record(s)\n";
}

// ── concurrent-add stress test ─────────────────────────────────────────────
// 8 threads each try to add 5 students whose IDs overlap heavily.
// Expected: exactly one thread wins each ID; all duplicates are rejected.
// Verifies shared_mutex prevents data corruption under concurrent writes.
static void runConcurrentTest(StudentRepository& repo) {
    std::cout << "\n[CONCURRENT TEST] 8 threads x 5 adds, IDs 200-204 (heavy overlap)\n";

    std::atomic<int> accepted{0};
    std::atomic<int> rejected{0};

    auto worker = [&](int threadId) {
        for (int i = 200; i <= 204; ++i) {
            Student s{i, "Thread" + std::to_string(threadId), 20 + threadId, "B"};
            if (repo.addStudent(s)) ++accepted;
            else                    ++rejected;
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t)
        threads.emplace_back(worker, t);
    for (auto& th : threads)
        th.join();

    std::cout << "  Accepted: " << accepted << "  (expected 5 — one per unique ID)\n";
    std::cout << "  Rejected: " << rejected << "  (expected 35 — all duplicates)\n";
    std::cout << "  IDs 200-204 in repo:\n";
    for (int i = 200; i <= 204; ++i) {
        auto r = repo.findById(i);
        if (r) printStudent(*r);
    }

    // Clean up the test records so normal menu state is unchanged
    for (int i = 200; i <= 204; ++i) repo.deleteStudent(i);
    std::cout << "  (test records removed)\n";
}

// ── menu ───────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    const std::string csvPath = (argc > 1) ? argv[1] : "students.csv";

    StudentRepository repo;
    repo.loadFromCSV(csvPath);
    std::cout << "Loaded from " << csvPath << "\n";

    while (true) {
        std::cout << "\n1) List   2) Search   3) Add   4) Update   "
                     "5) Delete   6) Sort   7) Concurrent test   0) Quit\n> ";

        int choice;
        if (!(std::cin >> choice)) break;
        std::cin.ignore(1000, '\n');    // flush remainder of input line

        if (choice == 0) break;

        // ── LIST ──────────────────────────────────────────────────────────
        if (choice == 1) {
            printList(repo.getAll());

        // ── SEARCH ────────────────────────────────────────────────────────
        } else if (choice == 2) {
            std::cout << "Name (substring, case-insensitive): ";
            std::string q; std::getline(std::cin, q);
            printList(repo.findByName(q));

        // ── ADD ───────────────────────────────────────────────────────────
        } else if (choice == 3) {
            Student s;
            std::cout << "ID: ";    std::cin >> s.id;
            std::cin.ignore(1000, '\n');
            std::cout << "Name: ";  std::getline(std::cin, s.name);
            std::cout << "Age: ";   std::cin >> s.age;
            std::cin.ignore(1000, '\n');
            std::cout << "Grade: "; std::getline(std::cin, s.grade);

            if (repo.addStudent(s))
                std::cout << "Added.\n";
            else
                std::cout << "ERROR: ID " << s.id << " already exists.\n";

        // ── UPDATE ────────────────────────────────────────────────────────
        } else if (choice == 4) {
            Student s;
            std::cout << "ID to update: "; std::cin >> s.id;
            std::cin.ignore(1000, '\n');

            auto existing = repo.findById(s.id);
            if (!existing) { std::cout << "ERROR: ID " << s.id << " not found.\n"; continue; }

            std::cout << "Current: ";
            printHeader(); printStudent(*existing);

            std::cout << "New name  (blank = keep \"" << existing->name  << "\"): ";
            std::string tmp; std::getline(std::cin, tmp);
            s.name  = tmp.empty() ? existing->name  : tmp;

            std::cout << "New age   (0 = keep " << existing->age << "): ";
            std::cin >> s.age; std::cin.ignore(1000, '\n');
            if (s.age == 0) s.age = existing->age;

            std::cout << "New grade (blank = keep \"" << existing->grade << "\"): ";
            std::getline(std::cin, tmp);
            s.grade = tmp.empty() ? existing->grade : tmp;

            if (repo.updateStudent(s))
                std::cout << "Updated.\n";
            else
                std::cout << "ERROR: update failed.\n";

        // ── DELETE ────────────────────────────────────────────────────────
        } else if (choice == 5) {
            std::cout << "ID to delete: ";
            int id; std::cin >> id; std::cin.ignore(1000, '\n');

            if (repo.deleteStudent(id))
                std::cout << "Deleted.\n";
            else
                std::cout << "ERROR: ID " << id << " not found.\n";

        // ── SORT ──────────────────────────────────────────────────────────
        } else if (choice == 6) {
            std::cout << "Sort by: 1) ID  2) Name  3) Age  4) Grade\n> ";
            int f; std::cin >> f; std::cin.ignore(1000, '\n');

            SortField field = SortField::ID;
            if      (f == 2) field = SortField::NAME;
            else if (f == 3) field = SortField::AGE;
            else if (f == 4) field = SortField::GRADE;

            printList(repo.sortBy(field));
            std::cout << "  (original order unchanged — this was a sorted copy)\n";

        // ── CONCURRENT TEST ───────────────────────────────────────────────
        } else if (choice == 7) {
            runConcurrentTest(repo);

        } else {
            std::cout << "Unknown option.\n";
        }
    }

    std::cout << "Bye.\n";
    return 0;
}
