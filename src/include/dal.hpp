#pragma once
#include "sqlite3.h"
#include "crow.h"
#include <string>
#include <vector>
#include <variant>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

using namespace std;

// A single bound SQL parameter. Remembers the type it was constructed from so
// binding can dispatch to the matching sqlite3_bind_* call instead of pushing
// everything through bind_text.
class DbValue {
public:
	DbValue(nullptr_t)     : v(nullptr) {}
	DbValue(int i)         : v(static_cast<int64_t>(i)) {}
	DbValue(int64_t i)     : v(i) {}
	DbValue(double d)      : v(d) {}
	DbValue(string s)      : v(move(s)) {}
	DbValue(const char* s) : v(string(s)) {}

	void bind(sqlite3_stmt* stmt, int idx) const {
		visit([&](auto&& val) {
			using T = decay_t<decltype(val)>;
			if constexpr (is_same_v<T, nullptr_t>)    sqlite3_bind_null(stmt, idx);
			else if constexpr (is_same_v<T, int64_t>) sqlite3_bind_int64(stmt, idx, val);
			else if constexpr (is_same_v<T, double>)  sqlite3_bind_double(stmt, idx, val);
			// TRANSIENT: sqlite copies the text, so it stays valid even if the
			// caller's params vector was a temporary.
			else sqlite3_bind_text(stmt, idx, val.c_str(), -1, SQLITE_TRANSIENT);
		}, v);
	}

private:
	variant<nullptr_t, int64_t, double, string> v;
};

using DbParams = vector<DbValue>;

inline void bindParams(sqlite3_stmt* stmt, const DbParams& params) {
	for (size_t i = 0; i < params.size(); i++) {
		params[i].bind(stmt, static_cast<int>(i + 1));
	}
}

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
	int user_id;
	string session_token;

	static SessionModel fromRow(sqlite3_stmt* stmt) {
		SessionModel session;
		session.id = sqlite3_column_int(stmt, 0);
		session.session_token
		       	= reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
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

	static SharedlistModel fromRow(sqlite3_stmt* stmt) {
		SharedlistModel sharedlist;
		sharedlist.id = sqlite3_column_int(stmt, 0);
		sharedlist.owner_id = sqlite3_column_int(stmt, 1);
		sharedlist.origin_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		sharedlist.origin_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		sharedlist.spotify_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
		sharedlist.apple_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
		return sharedlist;
	}
};

struct TrackModel {
	int id;
	string spotify_id;
	string apple_id;
	string name;
	string artists;
	string album;

	static TrackModel fromRow(sqlite3_stmt* stmt) {
		TrackModel track;
		track.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		track.artists = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		track.album = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		track.spotify_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		return track;
	}
};

struct SharedlistTrackModel {
        string id;
        string name;
        vector<string> artists;
        string album;
        string albumId;


        static SharedlistTrackModel fromJson(const crow::json::rvalue& item) {
                SharedlistTrackModel track;
                track.id = item["id"].s();
                track.name = item["name"].s();
                track.album = item["album"]["name"].s();
                track.albumId = item["album"]["id"].s();

                for (auto& artist : item["artists"]) {
                        track.artists.push_back(artist["name"].s());
                }

                return track;
		}
};

class Database { 
public:
	Database(const string& databaseName = "/data/sharedlist/database/sharedlist.db");
	~Database();

	bool open();
	bool execute(const string& sql) const;
	// Returns 0 on success, 1 on SQL error. Callers that hand back a value of
	// their own (an id, a count) must translate a 1 into -1 so the error is
	// never mistaken for a real result.
	int prepareStatement(const string& sql, const DbParams& params = {}) const;

	template<typename T>
	vector<T> query(const string& sql, const DbParams& params = {}) const {
		sqlite3_stmt* stmt = nullptr;
		vector<T> items;

		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
			cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
			return items;
		}
		bindParams(stmt, params);
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
