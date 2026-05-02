#pragma once
#include "dal.hpp"
#include "crypto.hpp"
#include <fstream>
#include <string>

using namespace std;

class SessionManager {
public:
	Database& db;
	Crypto& crypto;

	SessionManager(Database& _db, Crypto& _crypto);
	~SessionManager();

	int createSession(const string& username, const string& ip) const;
	int hasValidSession(const int id, const string& ip, const string& session_file, const string& username) const;
	vector<SessionModel> getSession(int user_id) const;
	vector<SessionModel> getSessionFromUsername(const string& username) const;
private:
	bool createSessionFile(const string& session_id, const string& username, const string& ip) const;
};
