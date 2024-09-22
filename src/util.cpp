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
	db->open();
	vector<string> params = {username};		
	const string sql = "select id, username, salt, hashword from users where username = ?";
	vector<UserModel> users = db->queryUsers(sql, params);
	if (users.empty() || users.size() > 1) {
		return 1;
	}
	cout << "users queried" << endl;
	const string session_query = "select id, session_id, user_id from sessions where user_id = ?";
	vector<string> user_params = {to_string(users[0].id)};
	vector<SessionModel> sessions = db->querySessions(session_query, user_params);
	if (sessions.size() > 1) {
		const string delete_sql = "delete from sessions where user_id = ?";
		//int result = db->prepareStatement(delete_sql, user_params); // result handling? idk brah
	}
	int session;
	if (sessions.size() == 0) {
		cout << "sessions size is zero" << endl;
		session = 1;
	} else {
		session = hasValidSession(users[0].id, ip, sessions[0].session_file, username);
	}
	cout << "session function ran" << endl;
	if (session) {
		cout << "in not session" << endl;
		string sessionId = generateSessionId();
		createSessionFile(sessionId, username, ip);
		const string sql2 = "insert into sessions (session_id, user_id) values (?, ?)";
		vector<string> params2 = {sessionId, to_string(users[0].id)};
		int result = db->prepareStatement(sql2, params2);
		db->close();
		return result;
	}
	return 0;
}

int Util::hasValidSession(const int id, const string& ip, const string& session_file, const string& username) {
	cout << "fn start" << endl;
	string filepath = "data/sessions/" + session_file + ".txt";
	ifstream file(filepath);
	cout << "file called" << endl;
	if (!file.is_open()) {
		cerr << "failed to open file: " << filepath << endl;
		return 1;
	}
	string line;
	int i = 0;
	string session_username;
	string session_ip;
	while(getline(file, line)) {
		cout << "while loop" << endl;
		switch (i) {
			case 0:
				session_username = line;
				break;
			case 1:
				session_ip = line;
				break;
			default:
				break;
		}
		i++;
	}
	if (ip == session_ip && username == session_username) {
		//all good babay
		return 0;
	} else {
		//oh no, create new session for ip? yes
		return 1;
	}
}
