#include <iostream>
#include "include/dal.hpp"

Database::Database(const std::string& databaseName) : db(nullptr), databaseName(databaseName), isOpen(false) {
}

Database::~Database() {
    close();
}

bool Database::open() {
    if (isOpen) {
        std::cerr << "Database is already open." << std::endl;
        return false;
    }

    int result = sqlite3_open(databaseName.c_str(), &db);
    if (result != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    isOpen = true;
    return true;
}

void Database::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
        isOpen = false;
    }
}

bool Database::execute(const std::string& sql) {
    char* errorMessage = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorMessage);
    
    if (result != SQLITE_OK) {
        std::cerr << "SQL error: " << errorMessage << std::endl;
        sqlite3_free(errorMessage);
        return false;
    }

    return true;
}

std::vector<UserModel> Database::queryUsers(const std::string& sql) {
    sqlite3_stmt *stmt = nullptr;
    std::vector<UserModel> users;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return users; // Return empty vector on failure
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserModel user;
        user.id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        users.push_back(user);
    }

    sqlite3_finalize(stmt);
    return users; // Return the vector of users
}

sqlite3* Database::getDB() const {
    return db;
}
