#pragma once

#include <string>
#include "Student.h"

struct ValidationResult {
    bool        valid;
    std::string error;
};

class StudentValidator {
public:
    // Validate fields before they ever reach StudentRepository.
    // A message like {id:-1, name:"", age:999} should never reach the repo.
    static ValidationResult validate(const Student& s);
};
