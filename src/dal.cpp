#include <iostream>
#include "include/dal.hpp"

using namespace std;

Database::Database(const string& databaseName) : db(nullptr), databaseName(databaseName), isOpen(false) {
}

Database::~Database() {
    close();
}

bool Database::open() {
    if (isOpen) {
        cerr << "Database is already open." << endl;
        return false;
    }

    int result = sqlite3_open(databaseName.c_str(), &db);
    if (result != SQLITE_OK) {
        cerr << "Error opening database: " << sqlite3_errmsg(db) << endl;
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

bool Database::execute(const string& sql) {
    char* errorMessage = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorMessage);
    
    if (result != SQLITE_OK) {
        cerr << "SQL error: " << errorMessage << endl;
        sqlite3_free(errorMessage);
        return false;
    }

    return true;
}

vector<UserModel> Database::queryUsers(const string& sql) {
    sqlite3_stmt *stmt = nullptr;
    vector<UserModel> users;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
	UserModel user;
	user.id = -1;
	user.username = "Failed to prepare statement.";
	users.push_back(user);
	return users;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserModel user;
        user.id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        users.push_back(user);
    }

    sqlite3_finalize(stmt);
    return users; 
}

sqlite3* Database::getDB() const {
    return db;
}
