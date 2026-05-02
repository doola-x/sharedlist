#pragma once
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>

using namespace std;

struct PassComponents {
	string salt;
	string hashword;
};

class Crypto {
public:
	string generateSalt(size_t length) const;
	PassComponents hashPassword(const string& password) const;
	string hashword(const string& password, const string& salt) const;
	string generateSessionId() const;
private:
	string sha256(const string& str) const;
};
