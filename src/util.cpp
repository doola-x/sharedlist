#include <iostream>
#include "include/util.hpp"

using namespace std;

Util::Util() {
	this->db = new Database();
}


Util::~Util() {
	delete this->db;
}

string Util::generateSalt(size_t length) {
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

string Util::sha256(const string& str) {
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

PassComponents Util::hashPassword(const string& password) {
	string salt = generateSalt(16);
	string hashed = hashword(password, salt);
	PassComponents pc = {salt, hashed};
	return pc;
}

string Util::hashword(const string& password, const string& salt) {
	string hash = sha256(password + salt);
	for (int i = 0; i < 15; i++) {
		hash = sha256(hash);
	}
	return hash;
}

string Util::generateSessionId() {
	random_device rd;
	mt19937 generator(rd());
	uniform_int_distribution<> distribution(0, 15);

	stringstream ss;
	for (int i = 0; i < 32; i++) {
		ss << hex << distribution(generator);
	}

	return ss.str();
}

bool Util::createSessionFile(const string& session_id, const string& username, const string& ip) {
	ofstream session_file("data/sessions/" + session_id + ".txt");
	if (session_file.is_open()) {
		session_file << username << "\n" << ip;
		session_file.close();
		return true;
	}
	return false;
}

int Util::createSession(const string& username, const string& ip) {
	string sessionId = generateSessionId();
	bool result = createSessionFile(sessionId, username, ip);
	if (result) {
		cout << "in result is true" << endl;
		db->open();
		cout << "db is open" << endl;
		vector<string> params = {username};		
		const string sql1 = "select id, username, salt, hashword from users where username = ?";
		vector<UserModel> users = db->queryUsers(sql1, params);
		cout << "query ran" << endl;
		if (users.empty()) {
			return 1;
		}
		const string sql = "insert into sessions (session_id, user_id) values (?, ?)";
		vector<string> params2 = {sessionId, to_string(users[0].id)};
		int result = db->prepareStatement(sql, params2);
		db->close();
		return result;
	} else {
		return 1;
	}
}

int validSession()
