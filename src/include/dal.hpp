#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <tuple>

using namespace std;

struct UserModel {
    int id;
    string username;
};

class Database {
public:
    Database(const std::string& databaseName = "/data/sharedlist/database/sharedlist.db");
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool open();
    void close();
    bool execute(const std::string& sql);
    sqlite3* getDB() const;
    vector<UserModel> queryUsers(const std::string& sql);

private:
    sqlite3 *db;
    std::string databaseName;
    bool isOpen;
};
