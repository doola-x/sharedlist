#pragma once
#include "dal.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <curl/curl.h>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <random>
#include <string>

using namespace std;

struct PassComponents {
	string salt;
	string hashword;
};

class Util {
public:
	Database& db;

	Util(Database& _db);
	~Util();


	PassComponents hashPassword(const string& password) const;
	string hashword(const string& password, const string& salt) const;
	int createSession(const string& username, const string& ip) const;
	int hasValidSession(const int id, const string& ip, const string& session_file, const string& username) const;
	int recordState(string username, string state) const;
	string generateSalt(size_t length) const;
	vector<SessionModel> getSessionFromUsername(const string& username) const;
	vector<UserModel> getUser(const string& username) const;
	vector<SessionModel> getSession(int user_id)  const;
	string make_http_request(const string& url, const string& method, const string& post_data = "", const string& client_id = "", const string& client_secret = "", const string& access_token = "") const;
	string base64_encode(const string& input) const;
private:
	string sha256(const string& str) const;
	string generateSessionId() const;
	bool createSessionFile(const string& sessionId, const string& username, const string& ip) const;
};
