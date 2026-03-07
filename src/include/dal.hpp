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
	int valid;
};

struct TokensModel {
	int id;
	int user_id;
	string access_token;
	string refresh_token;
	string created_at;
};
struct SharedlistModel {
	int id;
	int owner_id;
	string spotify_id;
	string apple_id;
	string created_at;
};

class Database {
public:
	Database(const string& databaseName = "/data/sharedlist/database/sharedlist.db");
	~Database();

	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;

	bool open();
	sqlite3* getDB() const;
	bool execute(const string& sql);
	int prepareStatement(const string& sql, const vector<string>& params = {});
	vector<UserModel> queryUsers(const string& sql, const vector<string>& params = {});
	vector<SessionModel> querySessions(const string& sql, const vector<string>& params = {});
	vector<SpotifyStateModel> querySpotifyState(const string& sql, const vector<string>& params = {});
	vector<TokensModel> queryTokens(const string& sql, const vector<string>& params = {});
	vector<SharedlistModel> querySharedlists(const string& sql, const vector<string>& params = {});
	void close();
    

private:
	sqlite3 *db;
	string database_name;
	bool is_open;
};
