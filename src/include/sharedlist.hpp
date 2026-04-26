#pragma once
#include "dal.hpp"
#include "user.hpp"
#include "util.hpp"
#include "crow.h"

using namespace std;

class Sharedlist {
public:
	Database& db;
	User& user;
	Util& util;

	Sharedlist(Database& _db, User& _user, Util& _util);
	~Sharedlist();

	void addSharedlistTracks(string user_token, string origin_id) const;
	void syncSharedlistTracks(string user_token, string origin_id, int sharedlist_id) const;
	int createSharedlist(int user_id, string sharedlist_sp, string sharedlist_id) const;
};
