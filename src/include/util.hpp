#pragma once
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

class Util {
public:
	Util();
	~Util();

	Util(const Util&) = delete;
	Util& operator=(const Util&) = delete;

	string hashPassword(const string& password);

private:
	string generateSalt(size_t length);
	string sha256(const string& str);
	string hashword(const string& password, const string& salt);

};
