#include "Student.h"

nlohmann::json Student::toJson() const {
    return {
        {"id",    id},
        {"name",  name},
        {"age",   age},
        {"grade", grade}
    };
}

Student Student::fromJson(const nlohmann::json& j) {
    return Student{
        j.at("id").get<int>(),
        j.at("name").get<std::string>(),
        j.at("age").get<int>(),
        j.at("grade").get<std::string>()
    };
}
