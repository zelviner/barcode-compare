#include "role_mysql_dao.h"
#include "roles.hpp"

#include <immintrin.h>
#include <memory>
#include <vector>

RoleMysqlDao::RoleMysqlDao(const std::shared_ptr<zel::myorm::Database> &db)
    : db_(db) {
    init();
}

bool RoleMysqlDao::add(const std::shared_ptr<Role> &role) {
    auto roles           = Roles(*db_);
    roles["name"]        = role->name;
    roles["description"] = role->description;

    return roles.save();
}

std::vector<std::shared_ptr<Role>> RoleMysqlDao::all() {
    std::vector<std::shared_ptr<Role>> roles;

    auto all = Roles(*db_).all();
    for (auto one : all) {
        std::shared_ptr<Role> role = std::make_shared<Role>();

        role->id          = one("id").asInt();
        role->name        = one("name").asString();
        role->description = one("description").asString();

        roles.push_back(role);
    }

    return roles;
}

std::shared_ptr<Role> RoleMysqlDao::get(const int id) {
    std::shared_ptr<Role> role = std::make_shared<Role>();

    auto one          = Roles(*db_).where("id", id).one();
    role->id          = one("id").asInt();
    role->name        = one("name").asString();
    role->description = one("description").asString();

    return role;
}

void RoleMysqlDao::init() {
    // check if table exists
    if (!Roles(*db_).exists()) {
        // create table if not exists
        std::string sql = "CREATE TABLE IF NOT EXISTS roles ("
                          "id INT AUTO_INCREMENT PRIMARY KEY,"
                          "name VARCHAR(255) NOT NULL UNIQUE,"
                          "description VARCHAR(255)"
                          ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;";
        db_->execute(sql);

        // initialize role data
        std::shared_ptr<Role> user_admin = std::make_shared<Role>(Role{0, "SA", "用户管理员"});
        std::shared_ptr<Role> production = std::make_shared<Role>(Role{0, "PD", "生产"});
        std::shared_ptr<Role> qc         = std::make_shared<Role>(Role{0, "QC", "品质"});

        add(user_admin);
        add(production);
        add(qc);
    }
}