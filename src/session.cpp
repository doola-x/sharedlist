#include <iostream>
#include <chrono>
#include "include/session.hpp"

using namespace std;

SessionManager::SessionManager(Database& _db, Crypto& _crypto) : db(_db), crypto(_crypto) {}

SessionManager::~SessionManager() {}

vector<SessionModel> SessionManager::getSession(int user_id) const {
	const string sql = "select id, session_token, user_id from sessions where user_id = ?";
	DbParams params = {user_id};
	vector<SessionModel> sessions = db.query<SessionModel>(sql, params);
	if (sessions.size() > 1) {
		const string delete_sql = "delete from sessions where user_id = ?";
	}
	return sessions;
}

vector<SessionModel> SessionManager::getSessionFromUsername(const string& username) const {
	const string user_sql = "select id, username, salt, hashword from users where username = ?";
	DbParams user_params = {username};
	vector<UserModel> users = db.query<UserModel>(user_sql, user_params);
	return getSession(users[0].id);
}

string SessionManager::createSession(const string& username, const string& ip) const {
	const string user_sql = "select id, username, salt, hashword from users where username = ?";
	DbParams user_params = {username};
	vector<UserModel> users = db.query<UserModel>(user_sql, user_params);
	if (users.size() > 1 || users.empty()) {
		cerr << "users size is too large while creating session" << endl;
		return "";
	}

	vector<SessionModel> sessions = getSession(users[0].id);
	bool needs_session;
	if (sessions.size() == 0) {
		cout << "sessions size is zero" << endl;
		needs_session = true;
	} else {
		needs_session = hasValidSession(users[0].id, ip, sessions[0].session_token, username) == -1;
	}

	if (needs_session) {
		string session_token = crypto.generateSessionId();
 		auto time_point = chrono::utc_clock::now();
		auto duration = time_point.time_since_epoch();
		auto seconds = chrono::duration_cast<chrono::seconds>(duration);
		const string sql = "insert into sessions"
			"(session_token, user_id, expires) values (?, ?, ?)";
		DbParams params = {session_token, users[0].id, seconds.count() + 3600};
		return db.prepareStatement(sql, params) == 1 ? "" : session_token;
	}
	return "";
}

int SessionManager::hasValidSession(int id, const string& ip, const string& session_file, const string& username) const {
	DbParams values = {id, username}; 
	return 0;
}
