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
	Util();
	~Util();

	Util(const Util&) = delete;
	Util& operator=(const Util&) = delete;

	Database *db;

	char* exec(const char* cmd);
	PassComponents hashPassword(const string& password);
	string hashword(const string& password, const string& salt);
	int createSession(const string& username, const string& ip);
	int hasValidSession(const int id, const string& ip, const string& session_file, const string& username);
	string generateSalt(size_t length);
	vector<SessionModel> getSessionFromUsername(const string& username);
	vector<UserModel> getUserFromUsername(const string& username);
	string make_http_request(const string& url, const string& method, const string& post_data, const string& client_id, const string& client_secret);
	string make_http_request(const string& url, const string& method, const string& post_data);
	string base64_encode(const string& input);
private:
	string sha256(const string& str);
	string generateSessionId();
	bool createSessionFile(const string& sessionId, const string& username, const string& ip);
};
