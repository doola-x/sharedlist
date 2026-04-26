#include <iostream>
#include <thread>
#include "include/sharedlist.hpp"

using namespace std;

const string SPOTIFY_BASE_URL = "https://api.spotify.com/v1/";

Sharedlist::Sharedlist(Database& _db, User& _user, Util& _util)
	: db(_db), user(_user), util(_util) {}

Sharedlist::~Sharedlist() {}

int Sharedlist::createSharedlist(int user_id, string sharedlist_sp, string sharedlist_id) const {
        const string sql = "insert into sharedlists" 
		"(owner_id, origin_type, origin_id, spotify_id, apple_id)" 
		"values (?, ?, ?, 'null', 'null')";
	vector<string> params = {to_string(user_id), sharedlist_sp, sharedlist_id};

        int result = db.prepareStatement(sql, params);
	if (result == -1) {
		// bad insert error
	}

	const string& fetch_sql = "select id from sharedlists where origin_id = ?"; 
	vector<string> params = {sharedlist_id};

	return 0;
}

vector<SharedlistTrackModel> Sharedlist::fetchSpotifyTracks(
	string user_token, 
	string origin_id
) const {
	const string url = SPOTIFY_BASE_URL + "playlists/" + sharedlist_id + 
		"/items?fields=next,total,items(item(album(id,name),artists(id,name),id,name))";
	string response = util.make_http_request(url, "GET", "", "", "", user_token);

	auto tracks = crow::json::load(response);
	crow::json::rvalue next = tracks["next"];
	crow::json::rvalue total = tracks["total"];

	vector<SharedlistTrackModel> tracks_vec(total.i());
	vector<thread> threads;
	bool done = false;

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
}

void Sharedlist::addSharedlistTracks(string user_token, string origin_id) const {


	cout << "emplaced " << tracks_vec.size() << " items in vector" << endl;
	for (auto& track : tracks_vec) {
		threads.emplace_back(thread([track, this]() {
			const string& sql = "insert into tracks (origin_id, spotify_id) values"
				"(?, ?)";
			vector<string> params = {track.id, track.id, ""};
			int result = db.prepareStatement(sql, params);
		}));
	}
	for (auto& t : threads) t.join();
	cout << "threads finished" << endl;
}

void Sharedlist::syncSharedlistTracks(string user_token, string origin_id, int sharedlist_id) const {

}
