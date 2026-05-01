#include <iostream>
#include <thread>
#include "include/sharedlist.hpp"

using namespace std;

const string SPOTIFY_BASE_URL = "https://api.spotify.com/v1/";

Sharedlist::Sharedlist(shared_ptr<Database> _db, shared_ptr<User> _user, shared_ptr<Util> _util)
	: db(move(_db)), user(move(_user)), util(move(_util)) {}

Sharedlist::~Sharedlist() {}

int Sharedlist::createSharedlist(int user_id, string sharedlist_sp, string sharedlist_id) const {
        const string sql = "insert into sharedlists" 
		"(owner_id, origin_type, origin_id, spotify_id, apple_id)" 
		"values (?, ?, ?, '', '')";
	vector<string> params = {to_string(user_id), sharedlist_sp, sharedlist_id};

        int result = db->prepareStatement(sql, params);
	if (result == -1) {
		return result;
	}

	const string& fetch_sql = "select id, owner_id, origin_type, origin_id, spotify_id, apple_id"
				  " from sharedlists where origin_id = ?";
	vector<string> fetch_params = {sharedlist_id};
	vector<SharedlistModel> sharedlists = db->query<SharedlistModel>(fetch_sql, fetch_params); 
	cout << "created sharedlist id: " << sharedlists[0].id << endl;
	return sharedlists[0].id;
}

vector<SharedlistTrackModel> Sharedlist::fetchSpotifyTracks(
	string user_token, 
	string origin_id
) const {
	const string url = SPOTIFY_BASE_URL + "playlists/" + origin_id + 
		"/items?fields=next,total,items(item(album(id,name),artists(id,name),id,name))";
	string response = util->make_http_request(url, "GET", "", "", "", user_token);

	auto tracks = crow::json::load(response);
	crow::json::rvalue next = tracks["next"];
	crow::json::rvalue total = tracks["total"];

	vector<SharedlistTrackModel> tracks_vec(total.i());
	vector<thread> threads;
	bool done = false;

	while (!done) {
		for (auto& item : tracks["items"]) {
			if (auto val = item["item"]["id"]; val.t() == crow::json::type::Null) {
				cout << "track id is null, skipping" << endl;
				continue;
			}
			tracks_vec.emplace_back(SharedlistTrackModel::fromJson(item["item"]));
		}
		if (auto val = tracks["next"]; val.t() == crow::json::type::Null) {
			done = true;
			continue;
		} else {
			next = tracks["next"];
		}

		response = util->make_http_request(next.s(), "GET", "", "", "", user_token);
		tracks = crow::json::load(response);
	}

	return tracks_vec;
}

void Sharedlist::addSharedlistTracks(string user_token, string origin_id) const {
	vector<SharedlistTrackModel> tracks_vec = fetchSpotifyTracks(user_token, origin_id);
	cout << "emplaced " << tracks_vec.size() << " items in vector" << endl;
	vector<thread> threads(tracks_vec.size());
	for (auto& track : tracks_vec) {
		threads.emplace_back(thread([track, this]() {
			const string& sql = "insert into tracks (origin_id, spotify_id) values"
				"(?, ?)";
			vector<string> params = {track.id, track.id, ""};
			int result = db->prepareStatement(sql, params);
		}));
	}
	cout << "emplaced threads" << endl;
	for (auto& t : threads) t.join();
	cout << "threads finished" << endl;
}

void Sharedlist::syncSharedlistTracks(string user_token, string origin_id, int sharedlist_id) const {

}
