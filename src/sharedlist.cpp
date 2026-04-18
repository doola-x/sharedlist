#include <iostream>
#include <thread>
#include "include/sharedlist.hpp"

using namespace std;

const string SPOTIFY_BASE_URL = "https://api.spotify.com/v1/";

Sharedlist::Sharedlist() {
	this->db = new Database();
	this->user = new User();
	this->util = new Util();
	db->open();
}

Sharedlist::~Sharedlist() {
	db->close();
	delete this->db;
	delete this->user;
	delete this->util;
}

int Sharedlist::createSharedlist(int user_id, string sharedlist_sp, string sharedlist_id) {
	vector<string> params = {to_string(user_id), sharedlist_sp, sharedlist_id};
        const string sql = "insert into sharedlists (owner_id, origin_type, origin_id, spotify_id, apple_id) values (?, ?, ?, 'null', 'null')";
        int result = db->prepareStatement(sql, params);

	if (result == -1) {
		// bad insert error
	}

	string access_token = user->fetchToken(user_id);	
	return 0;
}

crow::json::rvalue Sharedlist::syncSharedlist(string user_token, string sharedlist_id) {
	const string url = SPOTIFY_BASE_URL + "playlists/" + sharedlist_id + 
		"/items?fields=next,items(item(album(id,name),artists(id,name),id,name))";
	cout << url << endl;
	string response = util->make_http_request(url, "GET", "", "", "", user_token);
	auto tracks = crow::json::load(response);
	cout << response << endl;
	cout << "loading items" << endl;
	for (auto& item : tracks["items"]) {
		cout << "creating track" << endl;
		SharedlistTrack track = SharedlistTrack::fromJson(item["item"]);
		cout << "created track " << track.id << endl;		
		thread([track]() {
				cout << "inserting to db etc" << endl;
		}).detach();
	}
	return tracks;
}
