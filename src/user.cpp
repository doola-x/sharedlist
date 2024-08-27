#include <iostream>
#include "include/user.hpp"
#include "include/dal.hpp"
#include "include/util.hpp"

using namespace std;

User::User() {
	this->id = -1;
	this->username = "";
	this->email = "";
	this->db = new Database();
}

User::~User() {
	delete this->db;
}

User::User(int _id, string _username, string _email) {
	this->id = _id;
	this->username = _username;
	this->email = _email;
	this->db = new Database();
}

int User::signupUser(const string& username, const string& hashword, const string& salt) {
	db->open();
	if (username.empty() || hashword.empty()) {
		return -1;
	}
	vector<string> params = {username, hashword, salt};
	const string sql = "insert into users (username, hashword, salt) values(? , ? , ?)";
	int result = db->prepareStatement(sql, params);
	db->close();
	return 0;
}

int User::loginUser(const string& username, const string& password) {
	db->open();
	vector<string> params = {username};
	const string sql = "select id, username, salt, hashword from users where username = ?";
	vector<UserModel> user = db->queryUsers(sql, params);
	if (user.empty()) {
		return -1;
	}
	Util *util = new Util();
	string testHash = util->hashword(password, user[0].salt);
	db->close();
	return testHash == user[0].hashword ? 0 : 1;
}

vector<UserModel> User::getUserName(const string& username) {
	db->open();
	vector<string> params = {username};
	const string sql = "select id, username, salt, hashword from users where username = ?";
	vector<UserModel> users = db->queryUsers(sql, params);
	if (users.empty()) {
		UserModel user;
		user.id = -1;
		user.username = "!";
		users.push_back(user);
	}
	db->close();
	return users;
}
