#pragma once
#include <curl/curl.h>
#include <string>

using namespace std;

class HttpClient {
public:
	string request(
		const string& url,
		const string& method,
		const string& post_data = "",
		const string& client_id = "",
		const string& client_secret = "",
		const string& access_token = ""
	) const;
private:
	string base64_encode(const string& input) const;
};
