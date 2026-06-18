#pragma once

#include <string>
#include "../../third_party/json.hpp"

struct Student {
    int         id;
    std::string name;
    int         age;
    std::string grade;

    nlohmann::json toJson() const;
    static Student fromJson(const nlohmann::json& j);
};
