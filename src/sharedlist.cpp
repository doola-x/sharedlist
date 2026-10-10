#include <iostream>
#include "include/sharedlist.hpp"

using namespace std;

const string SPOTIFY_BASE_URL = "https://api.spotify.com/v1/";

Sharedlist::Sharedlist(const Database* _db, const HttpClient* _http) : db(_db), http(_http) {}

Sharedlist::Sharedlist() {}

Sharedlist::~Sharedlist() {}

int Sharedlist::createSharedlist(int user_id, const string& origin_type, const string& origin_id) const {
	const string sql = "insert or ignore into sharedlists"
		" (owner_id, origin_type, origin_id, spotify_id, apple_id)"
		" values (?, ?, ?, '', '')";
	DbParams params = {user_id, origin_type, origin_id};

	if (db->prepareStatement(sql, params) == 1) {
		return -1;
	}

	const string fetch_sql = "select id, owner_id, origin_type, origin_id, spotify_id, apple_id from sharedlists"
		" where owner_id = ? and origin_id = ? order by id desc limit 1";
	DbParams fetch_params = {user_id, origin_id};
	vector<SharedlistModel> sharedlists = db->query<SharedlistModel>(fetch_sql, fetch_params);
	if (sharedlists.empty()) {
		cerr << "could not read back sharedlist for owner " << user_id
			<< " origin " << origin_id << endl;
		return -1;
	}
	cout << "created sharedlist id: " << sharedlists[0].id << endl;
	return sharedlists[0].id;
}

vector<SharedlistTrackModel> Sharedlist::fetchSpotifyTracks(
	const string& user_token,
	const string& origin_id
) const {
	const string url = SPOTIFY_BASE_URL + "playlists/" + origin_id +
		"/items?fields=next,total,items(item(album(id,name,release_date,images),artists(id,name),id,name,duration_ms,explicit))";
	string response = http->request(url, "GET", "", "", "", user_token);

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
		response = http->request(next.s(), "GET", "", "", "", user_token);
		tracks = crow::json::load(response);
	}

	return tracks_vec;
}

bool Sharedlist::fetchSpotifyPlaylistMeta(const string& user_token, const string& origin_id, SharedlistMetaModel& meta) const {
	const string url = SPOTIFY_BASE_URL + "playlists/" + origin_id +
		"?fields=name,description,snapshot_id,images(url),owner(display_name)";
	auto body = crow::json::load(http->request(url, "GET", "", "", "", user_token));
	if (!body || !body.has("name")) return false;
	meta = SharedlistMetaModel::fromJson(body);
	return true;
}

int Sharedlist::saveSharedlistMeta(int sharedlist_id, const SharedlistMetaModel& meta) const {
	const string sql = "update sharedlists set name = ?, description = ?, image_url = ?,"
		" origin_owner = ?, snapshot_id = ? where id = ?";
	DbParams params = {meta.name, meta.description, meta.image_url, meta.owner, meta.snapshot_id, sharedlist_id};
	return db->prepareStatement(sql, params);
}

int Sharedlist::addSharedlistTracks(const vector<SharedlistTrackModel>& tracks_vec) const {
	// upsert so tracks cached before the metadata columns existed get backfilled
	const string sql = "insert into tracks (origin_id, spotify_id, name, artists, album, album_id, duration_ms, image_url, explicit, release_date)"
		" values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
		" on conflict(origin_id) do update set name = excluded.name, artists = excluded.artists, album = excluded.album,"
		" album_id = excluded.album_id, duration_ms = excluded.duration_ms, image_url = excluded.image_url,"
		" explicit = excluded.explicit, release_date = excluded.release_date";

	int failed = 0;
	db->execute("BEGIN");
	for (auto& track : tracks_vec) {
		string artists_str;
		for (size_t i = 0; i < track.artists.size(); i++) {
			if (i > 0) artists_str += ", ";
			artists_str += track.artists[i];
		}
		DbParams params = {track.id, track.id, track.name, artists_str, track.album, track.albumId,
			static_cast<int64_t>(track.duration_ms), track.image_url, static_cast<int>(track.explicit_), track.release_date};
		if (db->prepareStatement(sql, params) == 1) failed++;
	}
	db->execute("COMMIT");

	if (failed) {
		cerr << "failed to insert " << failed << " of " << tracks_vec.size() << " tracks" << endl;
		return -1;
	}
	return 0;
}

bool Sharedlist::isOwner(int sharedlist_id, int user_id) const {
	const string sql = "select id, owner_id, origin_type, origin_id, coalesce(spotify_id, ''), coalesce(apple_id, '')"
		" from sharedlists where id = ? and owner_id = ?";
	DbParams params = {sharedlist_id, user_id};
	return db->query<SharedlistModel>(sql, params).size() == 1;
}

bool Sharedlist::getSharedlistInfo(int sharedlist_id, SharedlistInfoModel& info) const {
	const string sql =
		"select coalesce(sp.display_name, u.username), coalesce(sl.created_at, ''), sl.origin_type, sl.origin_id,"
		" (select count(*) from sharedlist_tracks st where st.sharedlist_id = sl.id),"
		" (select coalesce(sum(t.duration_ms), 0) from sharedlist_tracks st"
		"  join tracks t on t.origin_id = st.origin_id where st.sharedlist_id = sl.id),"
		" coalesce(sl.name, ''), coalesce(sl.description, ''), coalesce(sl.image_url, ''), coalesce(sl.origin_owner, '')"
		" from sharedlists sl join users u on u.id = sl.owner_id"
		" left join spotify_profiles sp on sp.user_id = sl.owner_id where sl.id = ?";
	DbParams params = {sharedlist_id};
	vector<SharedlistInfoModel> rows = db->query<SharedlistInfoModel>(sql, params);
	if (rows.size() != 1) return false;
	info = rows[0];
	return true;
}

vector<TrackModel> Sharedlist::getSharedlistTracks(int sharedlist_id, int offset, int limit) const {
	const string sql =
		"select t.name, t.artists, t.album, t.spotify_id,"
		" coalesce(t.duration_ms, 0), coalesce(t.image_url, ''), coalesce(t.explicit, 0)"
		" from tracks t"
		" join sharedlist_tracks st on st.origin_id = t.origin_id"
		" where st.sharedlist_id = ? order by t.id"
		" limit ? offset ?";
	DbParams params = {sharedlist_id, limit + 1, offset};
	return db->query<TrackModel>(sql, params);
}

int Sharedlist::syncSharedlistTracks(const vector<SharedlistTrackModel>& tracks_vec, int sharedlist_id) const {
	const string sql = "insert or ignore into sharedlist_tracks (sharedlist_id, origin_id)"
		" values (?, ?)";

	int failed = 0;
	db->execute("BEGIN");
	for (auto& track : tracks_vec) {
		DbParams params = {sharedlist_id, track.id};
		if (db->prepareStatement(sql, params) == 1) failed++;
	}
	db->execute("COMMIT");

	if (failed) {
		cerr << "failed to sync " << failed << " of " << tracks_vec.size()
			<< " tracks to sharedlist " << sharedlist_id << endl;
		return -1;
	}
	return 0;
}
