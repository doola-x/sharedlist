#pragma once
#include "dal.hpp"

using namespace std;

class User {
public:
	int id;
	string username;
	string email;
	Database *db;

	User();
	User(int _id, string _username, string _email);
	~User();

	string getUserName(int userId);
};