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
	
	static UserModel fromRow(sqlite3_stmt* stmt) {
		UserModel user;
		user.id = sqlite3_column_int(stmt, 0);
		user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		user.hashword = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		user.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		return user;
	}
};
struct SessionModel {
	int id;
	string session_file;
	int user_id;

	static SessionModel fromRow(sqlite3_stmt* stmt) {
		SessionModel session;
		session.id = sqlite3_column_int(stmt, 0);
		session.session_file = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		session.user_id = sqlite3_column_int(stmt, 2);
		return session;
	}
};
struct SpotifyStateModel {
	int id;
	int user_id;
	string state;
	string created_at;
	int valid;

	static SpotifyStateModel fromRow(sqlite3_stmt* stmt){
		SpotifyStateModel state;
		state.id = sqlite3_column_int(stmt, 0);
		state.user_id = sqlite3_column_int(stmt, 1);
		state.state = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		state.valid = sqlite3_column_int(stmt, 3);
		return state;
	}
};
struct TokenModel {
	int id;
	int user_id;
	string access_token;
	string refresh_token;
	string created_at;

	static TokenModel fromRow(sqlite3_stmt* stmt) {
		TokenModel token;
		token.id = sqlite3_column_int(stmt, 0);
		token.user_id = sqlite3_column_int(stmt, 1);
		token.access_token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		token.refresh_token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		return token;
	}
};
struct SharedlistModel {
	int id;
	int owner_id;
	string origin_type;
	string origin_id;
	string spotify_id;
	string apple_id;
	string created_at;
};
struct TrackModel {
	int id;
	string spotify_id;
	string apple_id;
};
struct SharedistTrackModel {
	int id;
	int sharedlist_id;
	int track_id;
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

	template<typename T>
	vector<T> query(const string& sql, const vector<string>& params = {}) {
		sqlite3_stmt* stmt = nullptr;
		vector<T> items;

		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
			cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
			return items;
		}
		if (params.empty()) {
			cout << "Params is empty, exiting." << endl;
		}

		for (int i = 1; i <= params.size(); i++) {
			sqlite3_bind_text(stmt, i, params[i-1].c_str(), -1, SQLITE_STATIC);
		}
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			items.push_back(T::fromRow(stmt));
		}

		sqlite3_finalize(stmt);
		return items;
	}
	void close();
    

private:
	sqlite3 *db;
	string database_name;
	bool is_open;
};
