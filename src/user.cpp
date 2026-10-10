#include <iostream>
#include <stdexcept>
#include "dal.hpp"
#include "include/user.hpp"

using namespace std;

UserModel User::getUser(const string& username) const {
	DbParams params = {username};
	const string sql = "select id, username, salt, hashword from users where username = ?";

	vector<UserModel> users = db->query<UserModel>(sql, params);
    if (users.size() > 1 || users.size() == 0) {
        throw std::logic_error("conflict! more than one or no users.");
    }

    return users[0];
}

int User::signupUser(const string& username, const string& hashword, const string& salt) const {
	if (username.empty() || hashword.empty()) {
		return -1;
	}
	DbParams params = {username, hashword, salt};
	const string sql = "insert into users (username, hashword, salt) values(?, ?, ?)";
	return db->prepareStatement(sql, params);
}

int User::loginUser(const string& username, const string& password) const {
	UserModel user = getUser(username);
	if (!user.id) {
		cerr << "results were empty during signin for user " << username << endl;
		return -1;
	}
	string testHash = crypto->hashword(password, user.salt);
	return testHash == user.hashword ? 0 : -1;
}

int User::recordState(const string& username, const string& state) const {
	UserModel user = getUser(username);
	DbParams params = {user.id, state};
	const string sql = "insert into spotify_state (user_id, state, valid) values (?, ?, 1)";
	return db->prepareStatement(sql, params);
}

SpotifyStateModel User::fetchState(const string& state) const {
	DbParams params = {state};
	const string sql = "select id, user_id, state, created_at, valid from spotify_state where state = ? and valid = 1";
	vector<SpotifyStateModel> states = db->query<SpotifyStateModel>(sql, params);
	return states[0];
}

int User::recordToken(int user_id, const string& state, const string& token, const string& refresh_token) const {
    std::cout << "refresh: " << refresh_token << std::endl;
	DbParams params = {user_id, token, refresh_token};
	const string sql = "insert into tokens (user_id, access_token, refresh_token) values (?, ?, ?)";
	int result = db->prepareStatement(sql, params);
	if (result == 1) return result;
	const DbParams& update_params = {user_id};
	const string update_sql = "update spotify_state set valid = 0 where user_id = ?";
	return db->prepareStatement(update_sql, update_params);
}

TokenModel User::fetchToken(int user_id) const {
	DbParams params = {user_id};
	const string sql = "select id, user_id, access_token, refresh_token, created_at from tokens where user_id = ?";
	vector<TokenModel> tokens = db->query<TokenModel>(sql, params);
    if (tokens.size() > 1 || tokens.size() == 0) {
        cerr << "wrong token size!" << endl;
        return TokenModel();
    }

    return tokens[0];
}
