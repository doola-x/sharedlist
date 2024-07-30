#include <iostream>
#include "include/user.hpp"

using namespace std;

User::User() {
	this->id = -1;
	this->username = "";
	this->email = "";
}

User::~User() {
	delete this;
}

User::User(int _id, string _username, string _email) {
	this->id = _id;
	this->username = _username;
	this->email = _email;
}