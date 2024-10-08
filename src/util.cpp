#include <iostream>
#include "include/util.hpp"
#include <memory>
#include <stdexcept>
#include <cstring>


using namespace std;

Util::Util() {
	this->db = new Database();
}


Util::~Util() {
	delete this->db;
}

char* Util::exec(const char* cmd) {
    char buffer[128];
    char* result = new char[4096];  // Allocate a large enough buffer for the result.
    result[0] = '\0';  // Initialize the result as an empty string.
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");

    while (fgets(buffer, 128, pipe) != nullptr) {
        strcat(result, buffer);  // Append buffer contents to result.
    }

    pclose(pipe);
    return result;
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

static vector<UserModel> getUser(string username, Database &db) {
	vector<string> params = {username};		
	const string sql = "select id, username, salt, hashword from users where username = ?";
	vector<UserModel> users = db.queryUsers(sql, params);
	return users;
}

static vector<SessionModel> getSession(int user_id, Database &db) {
	const string session_query = "select id, session_id, user_id from sessions where user_id = ?";
	vector<string> user_params = {to_string(user_id)};
	vector<SessionModel> sessions = db.querySessions(session_query, user_params);
	if (sessions.size() > 1) {
		const string delete_sql = "delete from sessions where user_id = ?";
		//int result = db->prepareStatement(delete_sql, user_params); // result handling? idk brah
	}
	return sessions;
}

int Util::createSession(const string& username, const string& ip) {
	db->open();
	int session;	
	vector<UserModel> users = getUser(username, *db);
	vector<SessionModel> sessions = getSession(users[0].id, *db);
	if (sessions.size() == 0) {
		cout << "sessions size is zero" << endl;
		session = 1;
	} else {
		session = hasValidSession(users[0].id, ip, sessions[0].session_file, username);
	}
	if (session) {
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

vector<SessionModel> Util::getSessionFromUsername(const string& username) {
	db->open();
	vector<UserModel> users = getUser(username, *db);
	vector<SessionModel> sessions = getSession(users[0].id, *db);
	db->close();
	return sessions;
}

vector<UserModel> Util::getUserFromUsername(const string& username) {
	db->open();
	vector<UserModel> users = getUser(username, *db);
	db->close();
	return users;
}

int Util::hasValidSession(const int id, const string& ip, const string& session_file, const string& username) {
	string filepath = "data/sessions/" + session_file + ".txt";
	ifstream file(filepath);
	if (!file.is_open()) {
		cerr << "failed to open file: " << filepath << endl;
		return 1;
	}
	string line;
	int i = 0;
	string session_username;
	string session_ip;
	while(getline(file, line)) {
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
		return 0;
	} else {
		//oh no, create new session for ip? 
		return 1;
	}
}

static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    size_t totalSize = size * nmemb;
    out->append((char*)contents, totalSize);
    return totalSize;
}

string Util::make_http_request(const string& url, const string& method, const string& post_data, const string& client_id, const string& client_secret) {
    CURL* curl;
    CURLcode res;
    string response_data;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        // If the method is POST, set the appropriate curl options
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        }

        // Prepare the Authorization header (Base64-encoded client_id:client_secret)
        string credentials = client_id + ":" + client_secret;
        string encoded_credentials = base64_encode(credentials);
        string authorization_header = "Authorization: Basic " + encoded_credentials;

        // Prepare the Content-Type header
        string content_type_header = "Content-Type: application/x-www-form-urlencoded";

        // Initialize a curl_slist for headers
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, content_type_header.c_str());
        headers = curl_slist_append(headers, authorization_header.c_str());

        // Set the headers for the request
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Perform the request
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);  // Free the header list
    }

    curl_global_cleanup();

    return response_data;
}

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
string Util::base64_encode(const string& input) {
    std::string encoded_string;
    int in_len = input.size();
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(input.c_str() + (input.size() - in_len - 1));
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                encoded_string += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            encoded_string += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            encoded_string += '=';
    }

    return encoded_string;
}

