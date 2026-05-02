#pragma once
#include "dal.hpp"
#include "crypto.hpp"

using namespace std;

class User {
public:
	Database& db;
	Crypto& crypto;

	User(Database& _db, Crypto& _crypto);
	~User();

	vector<UserModel> getUser(const string& username) const;
	int signupUser(const string& username, const string& hashword, const string& salt) const;
	int loginUser(const string& username, const string& password) const;
	int recordState(const string& username, const string& state) const;
	int recordToken(int user_id, const string& state, const string& token) const;
	string fetchToken(int user_id) const;
	SpotifyStateModel fetchState(const string& state) const;
};
