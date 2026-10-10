#include <ctime>
#include <cstdlib>
#include "include/crow.h"
#include <iostream>
#include <stdexcept>
#include "dal.hpp"
#include "include/user.hpp"

using namespace std;

UserModel User::getUser(const string& username) const {
	DbParams params = {username};
	const string sql = "select id, username, salt, hashword from users where username = ?";

	vector<UserModel> users = db->query<UserModel>(sql, params);
    if (users.size() > 1 || users.size() == 0) {
        throw std::logic_error("conflict! more than one or no users.");
    }

    return users[0];
}

UserModel User::getUserById(int user_id) const {
	DbParams params = {user_id};
	const string sql = "select id, username, salt, hashword from users where id = ?";
	vector<UserModel> users = db->query<UserModel>(sql, params);
	return users.size() == 1 ? users[0] : UserModel{};
}

int User::signupUser(const string& username, const string& hashword, const string& salt) const {
	if (username.empty() || hashword.empty()) {
		return -1;
	}
	DbParams params = {username, hashword, salt};
	const string sql = "insert into users (username, hashword, salt) values(?, ?, ?)";
	return db->prepareStatement(sql, params);
}

int User::loginUser(const string& username, const string& password) const {
	UserModel user = getUser(username);
	if (!user.id) {
		cerr << "results were empty during signin for user " << username << endl;
		return -1;
	}
	string testHash = crypto->hashword(password, user.salt);
	return testHash == user.hashword ? 0 : -1;
}

int User::recordState(int user_id, const string& state) const {
	DbParams params = {user_id, state};
	const string sql = "insert into spotify_state (user_id, state, valid) values (?, ?, 1)";
	return db->prepareStatement(sql, params);
}

SpotifyStateModel User::fetchState(const string& state) const {
	DbParams params = {state};
	const string sql = "select id, user_id, state, created_at, valid from spotify_state where state = ? and valid = 1";
	vector<SpotifyStateModel> states = db->query<SpotifyStateModel>(sql, params);
	return states.empty() ? SpotifyStateModel{} : states[0];
}

int User::recordToken(int user_id, const string& state, const string& token, const string& refresh_token, int expires_in) const {
	// one row per user; re-auth overwrites the previous tokens
	DbParams params = {user_id, token, refresh_token, static_cast<int>(std::time(nullptr)) + expires_in};
	const string sql =
		"insert into tokens (user_id, access_token, refresh_token, expires_at) values (?, ?, ?, ?) "
		"on conflict(user_id) do update set access_token = excluded.access_token, "
		"refresh_token = excluded.refresh_token, expires_at = excluded.expires_at";
	int result = db->prepareStatement(sql, params);
	if (result == 1) return result;
	const DbParams& update_params = {user_id};
	const string update_sql = "update spotify_state set valid = 0 where user_id = ?";
	return db->prepareStatement(update_sql, update_params);
}

int User::saveSpotifyProfile(int user_id, const crow::json::rvalue& profile) const {
	DbParams params = {user_id, jsonString(profile, "id"), jsonString(profile, "display_name"),
		firstImageUrl(profile), jsonString(profile, "product"), jsonString(profile, "country")};
	const string sql =
		"insert into spotify_profiles (user_id, spotify_id, display_name, image_url, product, country) values (?, ?, ?, ?, ?, ?) "
		"on conflict(user_id) do update set spotify_id = excluded.spotify_id, display_name = excluded.display_name, "
		"image_url = excluded.image_url, product = excluded.product, country = excluded.country, "
		"updated_at = current_timestamp";
	return db->prepareStatement(sql, params);
}

TokenModel User::fetchToken(int user_id) const {
	DbParams params = {user_id};
	const string sql = "select id, user_id, access_token, refresh_token, expires_at from tokens where user_id = ?";
	vector<TokenModel> tokens = db->query<TokenModel>(sql, params);
    if (tokens.empty()) return TokenModel();

    return tokens[0];
}

string User::getValidAccessToken(int user_id, const HttpClient& http) const {
	TokenModel token = fetchToken(user_id);
	if (token.refresh_token.empty()) return "";
	if (token.expires_at - static_cast<long>(std::time(nullptr)) > 60) return token.access_token;

	const char* client_id = getenv("SPOTIFY_CLIENT_ID");
	const char* client_secret = getenv("SPOTIFY_CLIENT_SECRET");
	if (!client_id || !client_secret) {
		cerr << "SPOTIFY_CLIENT_ID/SECRET not set" << endl;
		return "";
	}

	// refresh tokens are opaque and may contain characters that need escaping
	char* escaped = curl_easy_escape(nullptr, token.refresh_token.c_str(), 0);
	string post_data = string("grant_type=refresh_token&refresh_token=") + (escaped ? escaped : "");
	curl_free(escaped);

	string response = http.request("https://accounts.spotify.com/api/token", "POST", post_data, client_id, client_secret);
	crow::json::rvalue body = crow::json::load(response);
	if (!body || !body.has("access_token") || !body.has("expires_in")) {
		cerr << "token refresh failed: " << response << endl;
		return "";
	}

	// Spotify only sometimes rotates the refresh token; keep the old one otherwise
	string new_refresh = body.has("refresh_token") ? string(body["refresh_token"].s()) : token.refresh_token;
	DbParams params = {string(body["access_token"].s()), new_refresh,
		static_cast<int>(std::time(nullptr)) + static_cast<int>(body["expires_in"].i()), user_id};
	const string sql = "update tokens set access_token = ?, refresh_token = ?, expires_at = ? where user_id = ?";
	if (db->prepareStatement(sql, params) == 1) return "";
	return body["access_token"].s();
}
