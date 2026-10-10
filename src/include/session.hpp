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

	// Replaces any existing sessions for the user and returns the new token ("" on failure).
	string createSession(int user_id) const;
	// Returns the owning user id for an unexpired token, or -1.
	int userIdFromToken(const string& token) const;
	void deleteSession(const string& token) const;
};
