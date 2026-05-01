#pragma once
#include "dal.hpp"
#include "user.hpp"
#include "util.hpp"
#include "crow.h"

using namespace std;

class Sharedlist {
public:
	shared_ptr<Database> db;
	shared_ptr<User> user;
	shared_ptr<Util> util;

	Sharedlist(shared_ptr<Database> _db, shared_ptr<User> _user, shared_ptr<Util> _util);
	~Sharedlist();

	vector<SharedlistTrackModel> fetchSpotifyTracks(string user_token, string origin_id) const;
	void addSharedlistTracks(string user_token, string origin_id) const;
	void syncSharedlistTracks(string user_token, string origin_id, int sharedlist_id) const;
	int createSharedlist(int user_id, string sharedlist_sp, string sharedlist_id) const;
};
