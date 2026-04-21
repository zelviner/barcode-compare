#include "user_sqlite_dao.h"
#include <SQLiteCpp/Statement.h>
#include <memory>
#include <vector>

UserSqliteDao::UserSqliteDao(const std::shared_ptr<SQLite::Database> &db)
    : db_(db) {
    init();
}

bool UserSqliteDao::login(const std::string &entered_name, const std::string &entered_password) {
    for (auto &user : all()) {
        if (user->name == entered_name && user->password == entered_password) {
            current_user_id_ = user->id;
            return true;
        }
    }

    return false;
}

bool UserSqliteDao::add(const std::shared_ptr<User> &user) {
    std::string       sql = "INSERT INTO users(name,description, password, role_id) VALUES(?,?,?,?)";
    SQLite::Statement insert(*db_, sql);
    insert.bind(1, user->name);
    insert.bind(2, user->description);
    insert.bind(3, user->password);
    insert.bind(4, user->role_id);

    return insert.exec();
}

std::vector<std::shared_ptr<User>> UserSqliteDao::all() {
    std::string       sql = "SELECT * FROM users";
    SQLite::Statement query(*db_, sql);

    std::vector<std::shared_ptr<User>> users;
    while (query.executeStep()) {
        std::shared_ptr<User> user = std::make_shared<User>();
        user->id                   = query.getColumn("id");
        user->name                 = query.getColumn("name").getString();
        user->description          = query.getColumn("description").getString();
        user->password             = query.getColumn("password").getString();
        user->role_id              = query.getColumn("role_id");

        users.push_back(user);
    }

    return users;
}

bool UserSqliteDao::remove(const int &id) {
    std::string       sql = "delete from users where id = ?";
    SQLite::Statement remove(*db_, sql);
    remove.bind(1, id);

    return remove.exec();
}

bool UserSqliteDao::clear() {
    std::string       sql = "DELETE FROM users";
    SQLite::Statement clear(*db_, sql);

    return clear.exec();
}

bool UserSqliteDao::update(const int &id, const std::shared_ptr<User> &user) {
    std::string       sql = "UPDATE users SET name = ?, description = ?, password = ?, role_id = ? WHERE id = ?";
    SQLite::Statement update(*db_, sql);
    update.bind(1, user->name);
    update.bind(2, user->description);
    update.bind(3, user->password);
    update.bind(4, user->role_id);
    update.bind(5, id);

    return update.exec();
}

std::shared_ptr<User> UserSqliteDao::currentUser() { return get(current_user_id_); }

std::shared_ptr<User> UserSqliteDao::get(const int &id) {
    std::string       sql = "SELECT * FROM users WHERE id = ?";
    SQLite::Statement query(*db_, sql);
    query.bind(1, id);

    if (query.executeStep()) {
        std::shared_ptr<User> user = std::make_shared<User>();
        user->id                   = query.getColumn("id");
        user->name                 = query.getColumn("name").getString();
        user->description          = query.getColumn("description").getString();
        user->password             = query.getColumn("password").getString();
        user->role_id              = query.getColumn("role_id");

        return user;
    }

    return nullptr;
}

bool UserSqliteDao::exists(const std::string &name) {
    std::string       sql = "SELECT name FROM users WHERE name = ?";
    SQLite::Statement exists(*db_, sql);
    exists.bind(1, name);

    if (exists.executeStep()) {
        return true;
    }
    return false;
}

void UserSqliteDao::init() {
    // check if table exists
    std::string       sql = "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'users'";
    SQLite::Statement qurey(*db_, sql);
    if (!qurey.executeStep()) {
        // create table if not exists
        sql = "CREATE TABLE IF NOT EXISTS users ("
              "id integer PRIMARY KEY AUTOINCREMENT,"
              "name TEXT NOT NULL,"
              "description TEXT NOT NULL,"
              "password TEXT NOT NULL,"
              "role_id integer NOT NULL);";
        db_->exec(sql);

        // initialize user data
        std::shared_ptr<User> admin = std::make_shared<User>(User{1, "admin", "管理员", "iflogic2025", 1});
        add(admin);
    }
}