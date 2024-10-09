#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <tuple>

using namespace std;

struct UserModel {
    int id;
    string username;
    string salt;
    string hashword;
};

struct SessionModel {
    int id;
    string session_file;
    int user_id;
};

struct SpotifyStateModel {
    int id;
    int user_id;
    string state;
    string created_at;
};

class Database {
public:
    Database(const std::string& databaseName = "/data/sharedlist/database/sharedlist.db");
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool open();
    void close();
    bool execute(const string& sql);
    sqlite3* getDB() const;
    vector<UserModel> queryUsers(const string& sql, const vector<string>& params = {});
    vector<SessionModel> querySessions(const string& sql, const vector<string>& params = {});
    vector<SpotifyStateModel> querySpotifyState(const string& sql, const vector<string>& params = {});
    int prepareStatement(const string& sql, const vector<string>& params = {});

private:
    sqlite3 *db;
    std::string databaseName;
    bool isOpen;
};
