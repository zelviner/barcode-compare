#include "user_mysql_dao.h"
#include "users.hpp"

#include <memory>
#include <vector>

UserMysqlDao::UserMysqlDao(const std::shared_ptr<zel::myorm::Database> &db)
    : db_(db) {
    init();
}

bool UserMysqlDao::login(const std::string &entered_name, const std::string &entered_password) {
    for (auto &user : all()) {
        if (user->name == entered_name && user->password == entered_password) {
            current_user_id_ = user->id;
            return true;
        }
    }

    return false;
}

bool UserMysqlDao::add(const std::shared_ptr<User> &user) {
    auto users           = Users(*db_);
    users["name"]        = user->name;
    users["description"] = user->description;
    users["password"]    = user->password;
    users["role_id"]     = user->role_id;

    return users.save();
}

std::vector<std::shared_ptr<User>> UserMysqlDao::all() {
    std::vector<std::shared_ptr<User>> users;

    auto all = Users(*db_).all();
    for (auto one : all) {
        std::shared_ptr<User> user = std::make_shared<User>();
        user->id                   = one("id").asInt();
        user->name                 = one("name").asString();
        user->description          = one("description").asString();
        user->password             = one("password").asString();
        user->role_id              = one("role_id").asInt();

        users.push_back(user);
    }

    return users;
}

bool UserMysqlDao::remove(const int &id) {
    auto one = Users(*db_).where("id", id).one();
    one.remove();

    return true;
}

bool UserMysqlDao::clear() {
    Users(*db_).remove();
    return true;
}

bool UserMysqlDao::update(const int &id, const std::shared_ptr<User> &user) {
    auto one           = Users(*db_).where("id", id).one();
    one["name"]        = user->name;
    one["description"] = user->description;
    one["password"]    = user->password;
    one["role_id"]     = user->role_id;

    return one.save();
}

std::shared_ptr<User> UserMysqlDao::currentUser() { return get(current_user_id_); }

std::shared_ptr<User> UserMysqlDao::get(const int &id) {
    std::shared_ptr<User> user = std::make_shared<User>();
    auto                  one  = Users(*db_).where("id", id).one();
    user->id                   = one("id").asInt();
    user->name                 = one("name").asString();
    user->description          = one("description").asString();
    user->password             = one("password").asString();
    user->role_id              = one("role_id").asInt();

    return user;
}

bool UserMysqlDao::exists(const std::string &name) {
    auto one = Users(*db_).where("name", name).one()["name"].asString();

    return one == "" ? false : true;
}

void UserMysqlDao::init() {
    // check if table exists
    if (!Users(*db_).exists()) {
        // create table if not exists
        std::string sql = "CREATE TABLE IF NOT EXISTS users ("
                          "id INT AUTO_INCREMENT PRIMARY KEY,"
                          "name VARCHAR(255) NOT NULL,"
                          "description VARCHAR(255) NOT NULL,"
                          "password VARCHAR(255) NOT NULL,"
                          "role_id INT NOT NULL"
                          ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;";
        db_->execute(sql);

        // initialize user data
        std::shared_ptr<User> admin = std::make_shared<User>(User{1, "admin", "管理员", "iflogic2025", 1});
        add(admin);
    }
}