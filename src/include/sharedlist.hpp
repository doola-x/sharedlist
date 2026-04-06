#pragma once
#include "dal.hpp"
#include "user.hpp"
#include "util.hpp"
#include "crow.h"

using namespace std;

class Sharedlist {
public:
	Database *db;
	User *user;
	Util *util;

	Sharedlist();
	~Sharedlist();

	crow::json::rvalue syncSharedlist(string user_token, string sharedlist_id);
	int createSharedlist(int user_id, string sharedlist_sp, string sharedlist_id);
};
