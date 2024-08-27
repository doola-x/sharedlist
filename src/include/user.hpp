#pragma once
#include "dal.hpp"
#include "util.hpp"

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

	vector<UserModel> getUserName(const string& username);
	int signupUser(const string& username, const string& hashword, const string& salt);
	int loginUser(const string& username, const string& password);
};
