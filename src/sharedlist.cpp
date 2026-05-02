#include <iostream>
#include <thread>
#include "include/sharedlist.hpp"

using namespace std;

const string SPOTIFY_BASE_URL = "https://api.spotify.com/v1/";

Sharedlist::Sharedlist(Database& _db, User& _user, HttpClient& _http) : db(_db), user(_user), http(_http) {}

Sharedlist::~Sharedlist() {}

int Sharedlist::createSharedlist(int user_id, const string& origin_type, const string& origin_id) const {
	const string sql = "insert into sharedlists"
		" (owner_id, origin_type, origin_id, spotify_id, apple_id)"
		" values (?, ?, ?, '', '')";
	vector<string> params = {to_string(user_id), origin_type, origin_id};

	int result = db.prepareStatement(sql, params);
	if (result == -1) {
		return result;
	}

	const string fetch_sql = "select id, owner_id, origin_type, origin_id, spotify_id, apple_id"
		" from sharedlists where origin_id = ?";
	vector<string> fetch_params = {origin_id};
	vector<SharedlistModel> sharedlists = db.query<SharedlistModel>(fetch_sql, fetch_params);
	cout << "created sharedlist id: " << sharedlists[0].id << endl;
	return sharedlists[0].id;
}

vector<SharedlistTrackModel> Sharedlist::fetchSpotifyTracks(
	const string& user_token,
	const string& origin_id
) const {
	const string url = SPOTIFY_BASE_URL + "playlists/" + origin_id +
		"/items?fields=next,total,items(item(album(id,name),artists(id,name),id,name))";
	string response = http.request(url, "GET", "", "", "", user_token);

	auto tracks = crow::json::load(response);
	crow::json::rvalue next = tracks["next"];
	crow::json::rvalue total = tracks["total"];

	vector<SharedlistTrackModel> tracks_vec;
	tracks_vec.reserve(total.i());
	bool done = false;

	while (!done) {
		for (auto& item : tracks["items"]) {
			if (auto val = item["item"]["id"]; val.t() == crow::json::type::Null || val.s() == "") {
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
		response = http.request(next.s(), "GET", "", "", "", user_token);
		tracks = crow::json::load(response);
	}

	return tracks_vec;
}

void Sharedlist::addSharedlistTracks(const vector<SharedlistTrackModel>& tracks_vec) const {
	const string sql = "insert into tracks (origin_id, spotify_id) values (?, ?)";

	db.execute("BEGIN");
	for (auto& track : tracks_vec) {
		vector<string> params = {track.id, track.id};
		db.prepareStatement(sql, params);
	}
	db.execute("COMMIT");
}

void Sharedlist::syncSharedlistTracks(const vector<SharedlistTrackModel>& tracks_vec, int sharedlist_id) const {
	const string sql = "insert or ignore into sharedlist_tracks (sharedlist_id, track_id)"
		" select ?, id from tracks where origin_id = ?";

	db.execute("BEGIN");
	for (auto& track : tracks_vec) {
		vector<string> params = {to_string(sharedlist_id), track.id};
		db.prepareStatement(sql, params);
	}
	db.execute("COMMIT");
}
