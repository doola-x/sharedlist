#pragma once
#include "dal.hpp"
#include "user.hpp"
#include "util.hpp"
#include "crow.h"

using namespace std;

struct SharedlistTrack {
	string id;
	string name;
	vector<string> artists;
	string album;
	string albumId;


	static SharedlistTrack fromJson(const crow::json::rvalue& item) {
		SharedlistTrack track;

		cout << "id" << endl;
		track.id = item["id"].s();
		cout << "name" << endl;
		track.name = item["name"].s();
		cout << "album" << endl;
		track.album = item["album"]["name"].s();
		track.albumId = item["album"]["id"].s();

		cout << "artists" << endl;
		for (auto& artist : item["artists"]) {
			track.artists.push_back(artist["name"].s());
		}	

		return track;
	}
};

class Sharedlist {
public:
	Database *db;
	User *user;
	Util *util;

	Sharedlist();
	~Sharedlist();

	crow::json::rvalue syncSharedlist(string user_token, string sharedlist_id);
	int createSharedlist(int user_id, string sharedlist_sp, string sharedlist_id);
};
