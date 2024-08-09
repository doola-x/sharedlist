#include <iostream>
#include "include/user.hpp"
#include "include/dal.hpp"

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

int User::signupUser(const string& username, const string& hashword) {
	db->open();
	if (username.empty() || hashword.empty()) {
		return -1;
	}
	vector<string> params = {username, hashword};
	const string sql = "insert into users (username, hashword) values(? , ?)";
	int result = db->prepareStatement(sql, params);
	db->close();
	return 0;
}

vector<UserModel> User::getUserName(int id) {
	this->db->open();
	vector<UserModel> users = db->queryUsers("select id, username from users");
	if (users.empty()) {
		UserModel user;
		user.id = -1;
		user.username = "!";
		users.push_back(user);
	}
	this->db->close();
	return users;
}
