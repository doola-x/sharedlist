#pragma once
#include "dal.hpp"
#include "util.hpp"

using namespace std;

class User {
public:
	Database& db;
	Util& util;

	User(Database& _db, Util& _util);
	~User();

	int signupUser(const string& username, const string& hashword, const string& salt) const;
	int loginUser(const string& username, const string& password) const;
	int recordState(string username, string state) const;
	int recordToken(int user_id, string state, string token) const;
	int createSharedlist(int user_id, const string& origin_type, const string& origin_id) const;
	string fetchToken(int user_id) const;
	SpotifyStateModel fetchState(string state) const; 

};
