#include <iostream>
#include "include/user.hpp"
#include "include/dal.hpp"
#include "include/util.hpp"
#include "include/sharedlist.hpp"

using namespace std;

User::User() {
	this->db = new Database();
	db->open();
}

User::~User() {
	db->close();
	delete this->db;
}

int User::signupUser(const string& username, const string& hashword, const string& salt) {
	if (username.empty() || hashword.empty()) {
		return -1;
	}
	vector<string> params = {username, hashword, salt};
	const string sql = "insert into users (username, hashword, salt) values(? , ? , ?)";
	int result = db->prepareStatement(sql, params);
	return 0;
}

int User::loginUser(const string& username, const string& password) {
	vector<string> params = {username};
	const string sql = "select id, username, salt, hashword from users where username = ?";
	vector<UserModel> user = db->query<UserModel>(sql, params);
	if (user.empty()) {
		return -1;
	}
	Util *util = new Util();
	string testHash = util->hashword(password, user[0].salt);
	return testHash == user[0].hashword ? 0 : 1;
}

int User::recordState(string username, string state) {
	Util *util = new Util();
	vector<UserModel> users = util->getUser(username);
	vector<string> params = {to_string(users[0].id), state};
	const string sql = "insert into spotify_state (user_id, state, valid) values (?, ?, 1)";
	int result = db->prepareStatement(sql, params);
	return result;
}

SpotifyStateModel User::fetchState(string state) {
	vector<string> params = {state};
	const string sql = "select id, user_id, state, created_at, valid from spotify_state where state = ? and valid = 1";
	vector<SpotifyStateModel> states = db->query<SpotifyStateModel>(sql, params);
	return states[0];
}

int User::recordToken(int user_id, string state, string token) {
	vector<string> params = {to_string(user_id), token, "null"};
	const string sql = "insert into tokens (user_id, access_token, refresh_token) values (?, ?, 'null')";
	int result = db->prepareStatement(sql, params);
	if (result == -1) return result;
	vector<string> params2 = {to_string(user_id)};
	const string sql2 = "update spotify_state set valid = 0 where user_id = ?";
	int result2 = db->prepareStatement(sql2, params2);
	return result2;
}

string User::fetchToken(int user_id) {
	vector<string> params = {to_string(user_id)};
	const string sql = "select id, user_id, access_token, refresh_token, created_at from tokens where user_id = ?";
	vector<TokenModel> tokens = db->query<TokenModel>(sql, params);
	return tokens[0].access_token;
}
