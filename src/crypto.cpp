#include "include/crypto.hpp"

using namespace std;

string Crypto::sha256(const string& str) const {
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, str.c_str(), str.size());
	SHA256_Final(hash, &sha256);

	stringstream ss;
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		ss << hex << setw(2) << setfill('0') << (int)hash[i];
	}
	return ss.str();
}

string Crypto::generateSalt(size_t length) const {
	unsigned char *buffer = new unsigned char[length];
	if (RAND_bytes(buffer, length) != 1) {
		delete[] buffer;
		return "!err!";
	}
	stringstream hexstream;
	hexstream << hex << setfill('0');
	for (size_t i = 0; i < length; i++) {
		hexstream << setw(2) << (int)buffer[i];
	}
	delete[] buffer;
	return hexstream.str();
}

PassComponents Crypto::hashPassword(const string& password) const {
	string salt = generateSalt(16);
	string hashed = hashword(password, salt);
	return {salt, hashed};
}

string Crypto::hashword(const string& password, const string& salt) const {
	string hash = sha256(password + salt);
	for (int i = 0; i < 15; i++) {
		hash = sha256(hash);
	}
	return hash;
}

string Crypto::generateSessionId() const {
	random_device rd;
	mt19937 generator(rd());
	uniform_int_distribution<> distribution(0, 15);
	stringstream ss;
	for (int i = 0; i < 32; i++) {
		ss << hex << distribution(generator);
	}
	return ss.str();
}
