#pragma once
#include <openssl/evp.h>

using namespace std;

class Util {
public:
	Util();
	~Util();

	Util(const Util&) = delete;
	Util& operator=(const Util&) = delete;

	string hashPassword(const string& password);

private:
	string hashword(const string& password, const string& salt);

};
