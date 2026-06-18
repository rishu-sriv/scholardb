#include <catch2/catch_test_macros.hpp>
#include "../src/common/CSVHandler.h"
#include <fstream>
#include <string>

static const std::string FIXTURE = "/tmp/scholar_fixture.csv";
static const std::string OUTFILE = "/tmp/scholar_roundtrip.csv";
static const std::string BADFILE = "/tmp/scholar_malformed.csv";

static void writeFixture() {
    std::ofstream f(FIXTURE);
    f << "id,name,age,grade\n"
      << "101,Alice Johnson,20,A\n"
      << "102,Bob Smith,22,B+\n"
      << "103,Carol White,19,A-\n";
}

TEST_CASE("CSVHandler: correct record count and field values", "[csv]") {
    writeFixture();
    auto students = CSVHandler::loadFromFile(FIXTURE);
    REQUIRE(students.size() == 3);

    CHECK(students[0].id    == 101);
    CHECK(students[0].name  == "Alice Johnson");
    CHECK(students[0].age   == 20);
    CHECK(students[0].grade == "A");

    CHECK(students[1].id    == 102);
    CHECK(students[1].name  == "Bob Smith");

    CHECK(students[2].grade == "A-");
}

TEST_CASE("CSVHandler: save + load round-trips without data loss", "[csv]") {
    std::vector<Student> original = {
        {101, "Alice Johnson", 20, "A"},
        {102, "Bob Smith",     22, "B+"},
        {103, "Carol White",   19, "A-"},
    };

    CSVHandler::saveToFile(OUTFILE, original);
    auto loaded = CSVHandler::loadFromFile(OUTFILE);

    REQUIRE(loaded.size() == original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        CHECK(loaded[i].id    == original[i].id);
        CHECK(loaded[i].name  == original[i].name);
        CHECK(loaded[i].age   == original[i].age);
        CHECK(loaded[i].grade == original[i].grade);
    }
}

TEST_CASE("CSVHandler: malformed lines are skipped, valid lines loaded", "[csv]") {
    {
        std::ofstream f(BADFILE);
        f << "id,name,age,grade\n"
          << "101,Alice,20,A\n"
          << "this,is,bad,data,extra_field\n"   // 5 fields — skipped
          << "not_a_number,Bob,22,B\n"          // non-int id — skipped
          << "103,Carol,19,A-\n";
    }
    auto students = CSVHandler::loadFromFile(BADFILE);
    // Only Alice (101) and Carol (103) should load
    REQUIRE(students.size() == 2);
    CHECK(students[0].id == 101);
    CHECK(students[1].id == 103);
}

TEST_CASE("CSVHandler: missing file returns empty vector without crashing", "[csv]") {
    auto students = CSVHandler::loadFromFile("/tmp/does_not_exist_scholar.csv");
    CHECK(students.empty());
}
