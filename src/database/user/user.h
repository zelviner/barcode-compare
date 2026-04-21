#pragma once

#include <string>

struct User {
    int         id;
    std::string name;
    std::string description;
    std::string password;
    int         role_id;
};