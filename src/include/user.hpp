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
	vector<SharedlistModel> getSharedlists(int id, const string& playlist_id, const string& provider);
	int signupUser(const string& username, const string& hashword, const string& salt);
	int loginUser(const string& username, const string& password);
	int recordState(string username, string state);
	int recordToken(int user_id, string state, string token);
	int createSharedlist(int user_id, const string& origin_type, const string& origin_id);
	string fetchToken(int user_id); 
	SpotifyStateModel fetchState(string state); 

};
