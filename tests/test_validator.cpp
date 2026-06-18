#include <catch2/catch_test_macros.hpp>
#include "../src/common/StudentValidator.h"

// ── rejection cases ───────────────────────────────────────────────────────

TEST_CASE("Validator: negative id is invalid", "[validator]") {
    Student s{-1, "Alice", 20, "A"};
    auto r = StudentValidator::validate(s);
    REQUIRE(!r.valid);
    CHECK(!r.error.empty());
}

TEST_CASE("Validator: zero id is invalid", "[validator]") {
    Student s{0, "Alice", 20, "A"};
    auto r = StudentValidator::validate(s);
    REQUIRE(!r.valid);
}

TEST_CASE("Validator: empty name is invalid", "[validator]") {
    Student s{1, "", 20, "A"};
    auto r = StudentValidator::validate(s);
    REQUIRE(!r.valid);
    CHECK(!r.error.empty());
}

TEST_CASE("Validator: age zero is out of range", "[validator]") {
    Student s{1, "Alice", 0, "A"};
    auto r = StudentValidator::validate(s);
    REQUIRE(!r.valid);
}

TEST_CASE("Validator: age above 120 is out of range", "[validator]") {
    Student s{1, "Alice", 121, "A"};
    auto r = StudentValidator::validate(s);
    REQUIRE(!r.valid);
}

TEST_CASE("Validator: empty grade is invalid", "[validator]") {
    Student s{1, "Alice", 20, ""};
    auto r = StudentValidator::validate(s);
    REQUIRE(!r.valid);
    CHECK(!r.error.empty());
}

// ── valid case ────────────────────────────────────────────────────────────

TEST_CASE("Validator: fully valid student passes all checks", "[validator]") {
    Student s{101, "Alice Johnson", 20, "A"};
    auto r = StudentValidator::validate(s);
    REQUIRE(r.valid);
    CHECK(r.error.empty());
}

TEST_CASE("Validator: boundary ages 1 and 120 are valid", "[validator]") {
    Student s{1, "Alice", 1, "A"};
    CHECK(StudentValidator::validate(s).valid);
    s.age = 120;
    CHECK(StudentValidator::validate(s).valid);
}
