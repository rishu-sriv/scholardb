#include <catch2/catch_test_macros.hpp>
#include "../src/repository/StudentRepository.h"
#include <thread>
#include <vector>
#include <atomic>

// ── add / getAll ──────────────────────────────────────────────────────────

TEST_CASE("Repository: add student and retrieve it", "[repo]") {
    StudentRepository repo;
    REQUIRE(repo.addStudent({101, "Alice", 20, "A"}));
    auto all = repo.getAll();
    REQUIRE(all.size() == 1);
    CHECK(all[0].id    == 101);
    CHECK(all[0].name  == "Alice");
    CHECK(all[0].age   == 20);
    CHECK(all[0].grade == "A");
}

TEST_CASE("Repository: duplicate ID is rejected", "[repo]") {
    StudentRepository repo;
    REQUIRE( repo.addStudent({101, "Alice", 20, "A"}));
    REQUIRE(!repo.addStudent({101, "Alice2", 21, "B"}));  // same ID
    CHECK(repo.getAll().size() == 1);
}

// ── update ────────────────────────────────────────────────────────────────

TEST_CASE("Repository: update replaces full record", "[repo]") {
    StudentRepository repo;
    repo.addStudent({101, "Alice", 20, "A"});
    REQUIRE(repo.updateStudent({101, "Alice Updated", 21, "A+"}));
    auto r = repo.findById(101);
    REQUIRE(r.has_value());
    CHECK(r->name  == "Alice Updated");
    CHECK(r->age   == 21);
    CHECK(r->grade == "A+");
}

TEST_CASE("Repository: update returns false for non-existent ID", "[repo]") {
    StudentRepository repo;
    REQUIRE(!repo.updateStudent({999, "Ghost", 25, "F"}));
}

// ── delete ────────────────────────────────────────────────────────────────

TEST_CASE("Repository: delete removes student and shrinks list", "[repo]") {
    StudentRepository repo;
    repo.addStudent({101, "Alice", 20, "A"});
    repo.addStudent({102, "Bob",   22, "B"});
    REQUIRE(repo.deleteStudent(101));
    CHECK(repo.getAll().size() == 1);
    CHECK(!repo.findById(101).has_value());
    CHECK( repo.findById(102).has_value());
}

TEST_CASE("Repository: delete returns false for non-existent ID", "[repo]") {
    StudentRepository repo;
    REQUIRE(!repo.deleteStudent(999));
}

// ── search ────────────────────────────────────────────────────────────────

TEST_CASE("Repository: findByName is case-insensitive substring match", "[repo]") {
    StudentRepository repo;
    repo.addStudent({101, "Alice Johnson", 20, "A"});
    repo.addStudent({102, "Bob Smith",     22, "B"});
    repo.addStudent({103, "Ali Baba",      30, "C"});

    auto r = repo.findByName("ali");   // matches Alice Johnson + Ali Baba
    REQUIRE(r.size() == 2);

    auto r2 = repo.findByName("SMITH");
    REQUIRE(r2.size() == 1);
    CHECK(r2[0].id == 102);

    CHECK(repo.findByName("nobody").empty());
}

// ── sort ──────────────────────────────────────────────────────────────────

TEST_CASE("Repository: sortBy returns sorted copy, original order unchanged", "[repo]") {
    StudentRepository repo;
    repo.addStudent({103, "Carol", 19, "A-"});
    repo.addStudent({101, "Alice", 20, "A"});
    repo.addStudent({102, "Bob",   22, "B"});

    auto byId = repo.sortBy(SortField::ID);
    REQUIRE(byId[0].id == 101);
    REQUIRE(byId[1].id == 102);
    REQUIRE(byId[2].id == 103);

    // Canonical order must still be insertion order (103, 101, 102)
    auto all = repo.getAll();
    CHECK(all[0].id == 103);
    CHECK(all[1].id == 101);
    CHECK(all[2].id == 102);
}

TEST_CASE("Repository: sortBy age ascending", "[repo]") {
    StudentRepository repo;
    repo.addStudent({101, "Alice", 22, "A"});
    repo.addStudent({102, "Bob",   19, "B"});
    repo.addStudent({103, "Carol", 20, "C"});

    auto sorted = repo.sortBy(SortField::AGE);
    CHECK(sorted[0].age == 19);
    CHECK(sorted[1].age == 20);
    CHECK(sorted[2].age == 22);
}

// ── concurrency ───────────────────────────────────────────────────────────

TEST_CASE("Repository: concurrent adds — no duplicates, no data loss", "[repo][concurrency]") {
    StudentRepository repo;

    // 8 threads each race to add IDs 200-204.
    // Exactly one thread must win each ID; all others are rejected.
    std::atomic<int> accepted{0};
    std::atomic<int> rejected{0};

    auto worker = [&](int threadId) {
        for (int id = 200; id <= 204; ++id) {
            Student s{id, "Thread" + std::to_string(threadId), 20, "B"};
            if (repo.addStudent(s)) ++accepted;
            else                    ++rejected;
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t)
        threads.emplace_back(worker, t);
    for (auto& th : threads)
        th.join();

    CHECK(accepted == 5);             // one winner per unique ID
    CHECK(rejected == 35);            // 8*5 - 5 = 35 duplicates rejected
    CHECK(repo.getAll().size() == 5);

    for (int id = 200; id <= 204; ++id)
        CHECK(repo.findById(id).has_value());
}
