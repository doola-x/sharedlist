#include <chrono>
#include "include/session.hpp"
#include "models.hpp"

using namespace std;

static int64_t nowSeconds() {
	return chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
}

SessionManager::SessionManager(const Database* _db, const Crypto* _crypto) : db(_db), crypto(_crypto) {}

SessionManager::~SessionManager() {}

string SessionManager::createSession(int user_id) const {
	// rotate: one live session per user, and sweep anything expired while we're here
	DbParams cleanup = {user_id, nowSeconds()};
	db->prepareStatement("delete from sessions where user_id = ? or expires <= ?", cleanup);

	string session_token = crypto->generateSessionId();
	DbParams params = {session_token, user_id, nowSeconds() + SESSION_TTL_SECONDS};
	const string sql = "insert into sessions (session_token, user_id, expires) values (?, ?, ?)";
	return db->prepareStatement(sql, params) == 1 ? "" : session_token;
}

int SessionManager::userIdFromToken(const string& token) const {
	if (token.empty()) return -1;
	DbParams params = {token, nowSeconds()};
	const string sql = "select id, session_token, user_id from sessions where session_token = ? and expires > ?";
	vector<SessionModel> sessions = db->query<SessionModel>(sql, params);
	return sessions.size() == 1 ? sessions[0].user_id : -1;
}

void SessionManager::deleteSession(const string& token) const {
	DbParams params = {token};
	db->prepareStatement("delete from sessions where session_token = ?", params);
}
