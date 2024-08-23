#pragma once
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
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

	PassComponents hashPassword(const string& password);
	string hashword(const string& password, const string& salt);

private:
	string sha256(const string& str);
	string generateSalt(size_t length);

};
