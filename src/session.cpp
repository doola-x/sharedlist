#include <iostream>
#include <chrono>
#include "include/session.hpp"

using namespace std;

SessionManager::SessionManager(Database& _db, Crypto& _crypto) : db(_db), crypto(_crypto) {}

SessionManager::~SessionManager() {}

vector<SessionModel> SessionManager::getSession(int user_id) const {
	const string sql = "select id, session_token, user_id from sessions where user_id = ?";
	vector<string> params = {to_string(user_id)};
	vector<SessionModel> sessions = db.query<SessionModel>(sql, params);
	if (sessions.size() > 1) {
		const string delete_sql = "delete from sessions where user_id = ?";
	}
	return sessions;
}

vector<SessionModel> SessionManager::getSessionFromUsername(const string& username) const {
	const string user_sql = "select id, username, salt, hashword from users where username = ?";
	vector<string> user_params = {username};
	vector<UserModel> users = db.query<UserModel>(user_sql, user_params);
	return getSession(users[0].id);
}

bool SessionManager::createSessionFile(const string& session_id, const string& username, const string& ip) const {
	ofstream session_file("data/sessions/" + session_id + ".txt");
	if (session_file.is_open()) {
		session_file << username << "\n" << ip;
		session_file.close();
		return true;
	}
	return false;
}

int SessionManager::createSession(const string& username, const string& ip) const {
	const string user_sql = "select id, username, salt, hashword from users where username = ?";
	vector<string> user_params = {username};
	vector<UserModel> users = db.query<UserModel>(user_sql, user_params);

	vector<SessionModel> sessions = getSession(users[0].id);
	int session;
	if (sessions.size() == 0) {
		cout << "sessions size is zero" << endl;
		session = 1;
	} else {
		session = hasValidSession(users[0].id, ip, sessions[0].session_token, username);
	}

	if (session) {
		string session_token = crypto.generateSessionId();
 		auto time_point = chrono::utc_clock::now();
		auto duration = time_point.time_since_epoch();
		auto seconds = chrono::duration_cast<chrono::seconds>(duration);
		const string sql = "insert into sessions" 
			"(session_token, user_id, expires) values (?, ?, ?)";
		vector<string> params = {session_token, to_string(users[0].id),};
		return db.prepareStatement(sql, params);
	}
	return 0;
}

int SessionManager::hasValidSession(const int id, const string& ip, const string& session_file, const string& username) const {
	string filepath = "data/sessions/" + session_file + ".txt";
	ifstream file(filepath);
	if (!file.is_open()) {
		cerr << "failed to open file: " << filepath << endl;
		return 1;
	}
	string line;
	int i = 0;
	string session_username;
	string session_ip;
	while (getline(file, line)) {
		switch (i) {
			case 0: session_username = line; break;
			case 1: session_ip = line; break;
			default: break;
		}
		i++;
	}
	if (ip == session_ip && username == session_username) {
		return 0;
	}
	return 1;
}
