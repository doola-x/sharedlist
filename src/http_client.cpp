#include <iostream>
#include "include/http_client.hpp"

using namespace std;

static size_t write_callback(void* contents, size_t size, size_t nmemb, string* out) {
	size_t totalSize = size * nmemb;
	out->append((char*)contents, totalSize);
	return totalSize;
}

string HttpClient::request(
	const string& url,
	const string& method,
	const string& post_data,
	const string& client_id,
	const string& client_secret,
	const string& access_token
) const {
	CURL* curl;
	CURLcode res;
	string response_data;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
		string authorization_header = "";
		string content_type_header = "";
		if (method == "POST") {
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
		}
		if (client_id != "" && client_secret != "") {
			string credentials = client_id + ":" + client_secret;
			string encoded_credentials = base64_encode(credentials);
			authorization_header = "Authorization: Basic " + encoded_credentials;
			content_type_header = "Content-Type: application/x-www-form-urlencoded";
		}
		if (access_token != "") {
			authorization_header = "Authorization: Bearer " + access_token;
		}
		struct curl_slist* headers = nullptr;
		if (content_type_header != "") headers = curl_slist_append(headers, content_type_header.c_str());
		if (authorization_header != "") headers = curl_slist_append(headers, authorization_header.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
	}

	curl_global_cleanup();
	return response_data;
}

static const string base64_chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

string HttpClient::base64_encode(const string& input) const {
	string encoded_string;
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
