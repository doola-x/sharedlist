#pragma once
#include "dal.hpp"
#include "user.hpp"
#include "http_client.hpp"
#include "crow.h"

using namespace std;

class Sharedlist {
public:
	Database& db;
	User& user;
	HttpClient& http;

	Sharedlist(Database& _db, User& _user, HttpClient& _http);
	~Sharedlist();

	vector<SharedlistTrackModel> fetchSpotifyTracks(const string& user_token, const string& origin_id) const;
	void addSharedlistTracks(const vector<SharedlistTrackModel>& tracks) const;
	void syncSharedlistTracks(const vector<SharedlistTrackModel>& tracks, int sharedlist_id) const;
	int createSharedlist(int user_id, const string& origin_type, const string& origin_id) const;
};
