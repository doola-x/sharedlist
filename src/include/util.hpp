#pragma once
#include "dal.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
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

	PassComponents hashPassword(const string& password);
	string hashword(const string& password, const string& salt);
	int createSession(const string& username, const string& ip);
	int validSession(const string& username);

private:
	string sha256(const string& str);
	string generateSalt(size_t length);
	string generateSessionId();
	bool createSessionFile(const string& sessionId, const string& username, const string& ip);
};
