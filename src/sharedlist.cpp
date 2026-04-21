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
		"/items?fields=next,total,items(item(album(id,name),artists(id,name),id,name))";
	string response = util.make_http_request(url, "GET", "", "", "", user_token);
	auto tracks = crow::json::load(response);

	vector<SharedlistTrackModel> tracks_vec;
	tracks_vec.reserve(tracks["total"].i());	
	
	bool done = false;
	crow::json::rvalue next = tracks["next"];
	while (!done) {
		for (auto& item : tracks["items"]) {
			tracks_vec.emplace_back(SharedlistTrackModel::fromJson(item["item"]));
		}
		if (auto val = tracks["next"]; val.t() == crow::json::type::Null) {
			done = true;
			continue;
		} else {
			next = tracks["next"];
		}

		response = util.make_http_request(next.s(), "GET", "", "", "", user_token);
		tracks = crow::json::load(response);
	}
	cout << "emplaced " << tracks_vec.size() << " items in vector" << endl;

	for (auto& tack : tracks_vec) {
		thread([track]() {
			const string& sql = "insert into tracks () values ";
		}
	}
	//thread([track]() {
	//	cout << "inserting to db track id" << track.id << endl;
	//}).detach();

	return tracks;
}
