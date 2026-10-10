#pragma once
#include "dal.hpp"
#include "crypto.hpp"
#include "models.hpp"
#include "http_client.hpp"

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
	int recordState(int user_id, const string& state) const;
	int recordToken(int user_id, const string& state, const string& token, const string& refresh_token, int expires_in) const;
	int saveSpotifyProfile(int user_id, const crow::json::rvalue& profile) const;
	TokenModel fetchToken(int user_id) const;
	string getValidAccessToken(int user_id, const HttpClient& http) const;
    UserModel getUser(const string& username) const;
    UserModel getUserById(int user_id) const;
	SpotifyStateModel fetchState(const string& state) const;
};
