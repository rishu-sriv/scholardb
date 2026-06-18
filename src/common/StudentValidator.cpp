#include "StudentValidator.h"

ValidationResult StudentValidator::validate(const Student& s) {
    if (s.id <= 0)
        return {false, "id must be a positive integer"};

    if (s.name.empty())
        return {false, "name must not be empty"};

    if (s.age <= 0 || s.age > 120)
        return {false, "age must be between 1 and 120"};

    if (s.grade.empty())
        return {false, "grade must not be empty"};

    return {true, ""};
}
