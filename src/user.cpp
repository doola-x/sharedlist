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

vector<UserModel> User::getUserName(int id) {
	this->db->open();
	vector<UserModel> users = db->queryUsers("select id, username from users");
	if (users.empty()) {
		UserModel user;
		user.id = -1;
		user.username = "!";
		users.push_back(user);
	}
	return users;
}
