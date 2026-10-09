#pragma once
#include <string>
#include "dal.hpp"
#include "crypto.hpp"
#include "models.hpp"

using namespace std;

class SessionManager {
public:
	const Database* db;
	const Crypto* crypto;

	SessionManager(const Database* _db, const Crypto* _crypto);
	~SessionManager();

	string createSession(const string& username, const string& ip) const;
	int hasValidSession(int id, const string& ip, const string& session_file, const string& username) const;
	SessionModel getSession(int user_id) const;
	SessionModel getSessionFromUsername(const string& username) const;
private:
	bool createSessionFile(const string& session_id, const string& username, const string& ip) const;
};
