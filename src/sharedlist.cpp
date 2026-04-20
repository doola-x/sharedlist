#include <iostream>
#include <thread>
#include "include/sharedlist.hpp"

using namespace std;

const string SPOTIFY_BASE_URL = "https://api.spotify.com/v1/";

Sharedlist::Sharedlist(Database& _db, User& _user, Util& _util)
	: db(_db), user(_user), util(_util) {}

Sharedlist::~Sharedlist() {}

int Sharedlist::createSharedlist(int user_id, string sharedlist_sp, string sharedlist_id) const {
	vector<string> params = {to_string(user_id), sharedlist_sp, sharedlist_id};
        const string sql = "insert into sharedlists (owner_id, origin_type, origin_id, spotify_id, apple_id) values (?, ?, ?, 'null', 'null')";
        int result = db.prepareStatement(sql, params);

	if (result == -1) {
		// bad insert error
	}

	string access_token = user.fetchToken(user_id);	
	return 0;
}

crow::json::rvalue Sharedlist::syncSharedlist(string user_token, string sharedlist_id) const {
	const string url = SPOTIFY_BASE_URL + "playlists/" + sharedlist_id + 
		"/items?fields=next,items(item(album(id,name),artists(id,name),id,name))";
	string response = util.make_http_request(url, "GET", "", "", "", user_token);
	auto tracks = crow::json::load(response);
	for (auto& item : tracks["items"]) {
		SharedlistTrackModel track = SharedlistTrackModel::fromJson(item["item"]);
		cout << "created track " << track.id << endl;		
		thread([track]() {
			cout << "inserting to db track id" << track.id << endl;
		}).detach();
	}
	return tracks;
}
