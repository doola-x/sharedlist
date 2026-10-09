#pragma once
#include "dal.hpp"
#include "crypto.hpp"
#include "models.hpp"

using namespace std;

class User {
public:
	const Database* db;
	const Crypto* crypto;

    User() : db(nullptr), crypto(nullptr) {}
	User(const Database* _db, const Crypto* _crypto) : db(_db), crypto(_crypto) {}
	~User() {}
    User operator=(const User& other) = delete;
    
	int signupUser(const string& username, const string& hashword, const string& salt) const;
	int loginUser(const string& username, const string& password) const;
	int recordState(const string& username, const string& state) const;
	int recordToken(int user_id, const string& state, const string& token) const;
	TokenModel fetchToken(int user_id) const;
    UserModel getUser(const string& username) const;
	SpotifyStateModel fetchState(const string& state) const;
};
