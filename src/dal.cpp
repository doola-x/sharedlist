#include <iostream>
#include "include/dal.hpp"

using namespace std;

Database::Database(const string& database_name) : db(nullptr), database_name(database_name), is_open(false) {}

Database::~Database() {
    close();
}

bool Database::open() {
    if (is_open) {
        cerr << "Database is already open." << endl;
        return false;
    }

    int result = sqlite3_open(database_name.c_str(), &db);
    if (result != SQLITE_OK) {
        cerr << "Error opening database: " << sqlite3_errmsg(db) << endl;
        return false;
    }
    is_open = true;
    return true;
}

void Database::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
        is_open = false;
    }
}

bool Database::execute(const string& sql) const {
    char* errorMessage = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorMessage);
    
    if (result != SQLITE_OK) {
        cerr << "SQL error: " << errorMessage << endl;
        sqlite3_free(errorMessage);
        return false;
    }

    return true;
}

int Database::prepareStatement(const string& sql, const DbParams& params) const {
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
	cerr << "Failded to prepare statement: " << sqlite3_errmsg(db) << endl;
    	return 1;
    }

    bindParams(stmt, params);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
	cerr << "error executing statement: " << sqlite3_errmsg(db) << endl;
	sqlite3_finalize(stmt);
	return 1;
    }

    sqlite3_finalize(stmt);

    return 0;
}
