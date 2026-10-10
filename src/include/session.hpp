#pragma once
#include <string>
#include "dal.hpp"
#include "crypto.hpp"
#include "models.hpp"

using namespace std;

class SessionManager {
public:
	static constexpr int SESSION_TTL_SECONDS = 3600;

	const Database* db;
	const Crypto* crypto;

	SessionManager(const Database* _db, const Crypto* _crypto);
	~SessionManager();

	string createSession(int user_id) const;
	int userIdFromToken(const string& token) const;
	void deleteSession(const string& token) const;
};
