#include <iostream>
#include "include/user.hpp"

using namespace std;

User::User(Database& _db, Crypto& _crypto) : db(_db), crypto(_crypto) {}

User::~User() {}

vector<UserModel> User::getUser(const string& username) const {
	const DbParams& params = {username};
	const string sql = "select id, username, salt, hashword from users where username = ?";
	return db.query<UserModel>(sql, params);
}

int User::signupUser(const string& username, const string& hashword, const string& salt) const {
	if (username.empty() || hashword.empty()) {
		return -1;
	}
	const DbParams& params = {username, hashword, salt};
	const string sql = "insert into users (username, hashword, salt) values(?, ?, ?)";
	return db.prepareStatement(sql, params);
}

int User::loginUser(const string& username, const string& password) const {
	vector<UserModel> user = getUser(username);
	if (user.empty()) {
		cerr << "results were empty during signin for user " << username << endl;
		return -1;
	}
	string testHash = crypto.hashword(password, user[0].salt);
	return testHash == user[0].hashword ? 0 : 1;
}

int User::recordState(const string& username, const string& state) const {
	vector<UserModel> users = getUser(username);
	if (users.size() > 1 || users.size() == 0) {
		cerr << "size issue, wrecked" << endl;
		return -1;
	}
	const DbParams& params = {users[0].id, state};
	const string sql = "insert into spotify_state (user_id, state, valid) values (?, ?, 1)";
	return db.prepareStatement(sql, params);
}

SpotifyStateModel User::fetchState(const string& state) const {
	const DbParams& params = {state};
	const string sql = "select id, user_id, state, created_at, valid from spotify_state where state = ? and valid = 1";
	vector<SpotifyStateModel> states = db.query<SpotifyStateModel>(sql, params);
	return states[0];
}

int User::recordToken(int user_id, const string& state, const string& token) const {
	const DbParams& params = {user_id, token, nullptr};
	const string sql = "insert into tokens (user_id, access_token, refresh_token) values (?, ?, ?)";
	int result = db.prepareStatement(sql, params);
	if (result == -1) return result;
	const DbParams& params2 = {user_id};
	const string sql2 = "update spotify_state set valid = 0 where user_id = ?";
	return db.prepareStatement(sql2, params2);
}

string User::fetchToken(int user_id) const {
	const DbParams& params = {user_id};
	const string sql = "select id, user_id, access_token, refresh_token, created_at from tokens where user_id = ?";
	vector<TokenModel> tokens = db.query<TokenModel>(sql, params);
	return tokens[0].access_token;
}
