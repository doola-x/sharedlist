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
	delete this;
}

User::User(int _id, string _username, string _email) {
	this->id = _id;
	this->username = _username;
	this->email = _email;
	this->db = new Database();
}

string User::getUserName(int id) {
	this->db->open();
	std::vector<UserModel> users = db->queryUsers("select id, username from users;");
    for (const auto& user : users) {
        std::cout << "User ID: " << user.id << ", Name: " << user.username << std::endl;
    }
}