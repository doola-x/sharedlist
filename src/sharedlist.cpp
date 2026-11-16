#include <iostream>
#include <algorithm>
#include <unordered_set>
#include "include/sharedlist.hpp"

using namespace std;

const string SPOTIFY_BASE_URL = "https://api.spotify.com/v1/";

Sharedlist::Sharedlist(const Database* _db, const HttpClient* _http) : db(_db), http(_http) {}

Sharedlist::Sharedlist() {}

Sharedlist::~Sharedlist() {}

// appends one row to the log; returns its id, or -1. batch_id 0 = none yet.
static long long logAction(const Database* db, int list_id, int actor_id, const Action& a,
		long long batch_id, const string& status, const string& reason) {
	const string sql = "insert into sharedlist_actions (sharedlist_id, actor_id, kind, payload, batch_id, status, reason)"
		" values (?, ?, ?, ?, ?, ?, ?)";
	DbValue batch = batch_id > 0 ? DbValue(static_cast<int64_t>(batch_id)) : DbValue(nullptr);
	DbParams params = {list_id, actor_id, kindName(a.kind), a.toJson(), batch, status, reason};
	if (db->prepareStatement(sql, params) == 1) return -1;
	return db->lastInsertId();
}

static string scalarString(const Database* db, const string& sql, const DbParams& params) {
	auto rows = db->query<StringModel>(sql, params);
	return rows.empty() ? "" : rows[0].value;
}

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
	int id = sharedlists[0].id;

	// first creation: the owner becomes a member, recorded in the log like any other action.
	// (add_member can't mint owners, so this bypasses apply() deliberately)
	if (roleOf(id, user_id).empty()) {
		db->execute("BEGIN IMMEDIATE");
		DbParams member = {id, user_id};
		Action a;
		a.kind = ActionKind::AddMember;
		a.user_id = user_id;
		a.new_value = "owner";
		if (db->prepareStatement("insert into sharedlist_members (sharedlist_id, user_id, role) values (?, ?, 'owner')", member) == 1
			|| logAction(db, id, user_id, a, 0, "applied", "") < 0) {
			db->execute("ROLLBACK");
			return -1;
		}
		db->execute("COMMIT");
	}
	cout << "created sharedlist id: " << id << endl;
	return id;
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

string Sharedlist::roleOf(int sharedlist_id, int user_id) const {
	return scalarString(db, "select role from sharedlist_members where sharedlist_id = ? and user_id = ?",
		{sharedlist_id, user_id});
}

vector<MemberModel> Sharedlist::getMembers(int sharedlist_id) const {
	const string sql = "select m.user_id, u.username, m.role, coalesce(m.added_at, '')"
		" from sharedlist_members m join users u on u.id = m.user_id"
		" where m.sharedlist_id = ? order by m.role = 'owner' desc, m.added_at, m.user_id";
	return db->query<MemberModel>(sql, {sharedlist_id});
}

int Sharedlist::findUserId(const string& username) const {
	auto rows = db->query<IntModel>("select id from users where username = ?", {username});
	return rows.empty() ? -1 : static_cast<int>(rows[0].value);
}

vector<ActionRecordModel> Sharedlist::history(int sharedlist_id, long long since_id, int limit) const {
	const string sql = "select a.id, a.actor_id, coalesce(u.username, ''), a.kind, a.payload,"
		" coalesce(a.batch_id, 0), a.status, coalesce(a.reason, ''), a.created_at"
		" from sharedlist_actions a left join users u on u.id = a.actor_id"
		" where a.sharedlist_id = ? and a.id > ? order by a.id limit ?";
	return db->query<ActionRecordModel>(sql, {sharedlist_id, static_cast<int64_t>(since_id), limit});
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
		" where st.sharedlist_id = ? order by st.position, st.id"
		" limit ? offset ?";
	DbParams params = {sharedlist_id, limit + 1, offset};
	return db->query<TrackModel>(sql, params);
}

static vector<string> loadOrder(const Database* db, int sharedlist_id) {
	auto rows = db->query<StringModel>(
		"select origin_id from sharedlist_tracks where sharedlist_id = ? order by position, id", {sharedlist_id});
	vector<string> order;
	order.reserve(rows.size());
	for (auto& r : rows) order.push_back(std::move(r.value));
	return order;
}

int Sharedlist::syncSharedlistTracks(const vector<SharedlistTrackModel>& tracks_vec, int sharedlist_id, int actor_id) const {
	vector<string> current = loadOrder(db, sharedlist_id);
	unordered_set<string> have(current.begin(), current.end());

	// origin may list a track twice; the list holds each once, first occurrence wins
	unordered_set<string> want;
	vector<string> want_order;
	for (auto& t : tracks_vec) {
		if (want.insert(t.id).second) want_order.push_back(t.id);
	}

	vector<Action> batch;
	for (auto& id : current) {
		if (!want.count(id)) {
			Action a;
			a.kind = ActionKind::RemoveTrack;
			a.track_id = id;
			batch.push_back(std::move(a));
		}
	}
	for (auto& id : want_order) {
		if (!have.count(id)) {
			Action a;
			a.kind = ActionKind::AddTrack;
			a.track_id = id;
			batch.push_back(std::move(a));
		}
	}
	if (batch.empty()) return 0;

	ApplyResult res = apply(sharedlist_id, actor_id, std::move(batch));
	if (!res.ok) {
		cerr << "failed to sync sharedlist " << sharedlist_id << ": " << res.error << endl;
		return -1;
	}
	int rejected = 0;
	for (auto& r : res.results) if (r.status == "rejected") rejected++;
	if (rejected) {
		cerr << "sync of sharedlist " << sharedlist_id << ": " << rejected << " of "
			<< res.results.size() << " actions rejected" << endl;
		return -1;
	}
	return 0;
}

Sharedlist::ApplyResult Sharedlist::apply(int list_id, int actor_id, vector<Action> actions) const {
	ApplyResult out;

	string role = roleOf(list_id, actor_id);
	if (role.empty()) {
		out.http = 403;
		out.error = "forbidden";
		return out;
	}
	for (auto& a : actions) {
		if (!canPerform(role, a, actor_id)) {
			out.http = 403;
			out.error = string("forbidden: ") + kindName(a.kind);
			return out;
		}
	}
	if (actions.empty()) {
		out.ok = true;
		return out;
	}

	if (!db->execute("BEGIN IMMEDIATE")) {
		out.http = 500;
		out.error = "db busy";
		return out;
	}

	// track order is loaded once, mutated in memory, and written back once at the end
	vector<string> order;
	unordered_set<string> have;
	bool loaded = false, dirty = false, fail = false;
	auto load = [&]() {
		if (loaded) return;
		order = loadOrder(db, list_id);
		have.insert(order.begin(), order.end());
		loaded = true;
	};
	auto exec = [&](const string& sql, const DbParams& p) {
		if (db->prepareStatement(sql, p) == 1) { fail = true; return false; }
		return true;
	};

	for (auto& a : actions) {
		string reason; // non-empty => rejected, state untouched

		if (!canPerform(role, a, actor_id)) {
			reason = "no longer a member"; // the actor left earlier in this batch
		} else switch (a.kind) {
		case ActionKind::AddTrack: {
			load();
			if (scalarString(db, "select origin_id from tracks where origin_id = ?", {a.track_id}).empty()) {
				reason = "unknown track";
			} else if (have.count(a.track_id)) {
				reason = "already in list";
			} else {
				int size = static_cast<int>(order.size());
				int pos = (a.position < 0 || a.position > size) ? size : a.position;
				order.insert(order.begin() + pos, a.track_id);
				have.insert(a.track_id);
				a.position = pos;
				dirty = true;
			}
			break;
		}
		case ActionKind::RemoveTrack: {
			load();
			auto it = find(order.begin(), order.end(), a.track_id);
			if (it == order.end()) {
				reason = "not in list";
			} else {
				a.position = static_cast<int>(it - order.begin());
				order.erase(it);
				have.erase(a.track_id);
				dirty = true;
			}
			break;
		}
		case ActionKind::MoveTrack: {
			load();
			auto it = find(order.begin(), order.end(), a.track_id);
			if (it == order.end()) {
				reason = "not in list";
				break;
			}
			int from = static_cast<int>(it - order.begin());
			int to = min(a.position, static_cast<int>(order.size()) - 1);
			if (to == from) {
				reason = "no change";
				break;
			}
			order.erase(it);
			order.insert(order.begin() + to, a.track_id);
			a.from = from;
			a.position = to;
			dirty = true;
			break;
		}
		case ActionKind::ChangeMeta: {
			const char* col = a.field == "name" ? "name"
				: a.field == "description" ? "description"
				: a.field == "image_url" ? "image_url" : nullptr;
			if (!col) {
				reason = "unknown field";
				break;
			}
			string old = scalarString(db, string("select coalesce(") + col + ", '') from sharedlists where id = ?", {list_id});
			if (old == a.new_value) {
				reason = "no change";
				break;
			}
			exec(string("update sharedlists set ") + col + " = ? where id = ?", {a.new_value, list_id});
			a.old_value = old;
			break;
		}
		case ActionKind::AddMember: {
			if (!isAssignableRole(a.new_value)) {
				reason = "invalid role";
			} else if (scalarString(db, "select username from users where id = ?", {a.user_id}).empty()) {
				reason = "no such user";
			} else if (!roleOf(list_id, a.user_id).empty()) {
				reason = "already a member";
			} else {
				exec("insert into sharedlist_members (sharedlist_id, user_id, role) values (?, ?, ?)",
					{list_id, a.user_id, a.new_value});
			}
			break;
		}
		case ActionKind::RemoveMember: {
			string target = roleOf(list_id, a.user_id);
			if (target.empty()) {
				reason = "not a member";
			} else if (target == "owner") {
				reason = "cannot remove the owner";
			} else if (exec("delete from sharedlist_members where sharedlist_id = ? and user_id = ?", {list_id, a.user_id})) {
				a.old_value = target;
				if (a.user_id == actor_id) role = "";
			}
			break;
		}
		case ActionKind::ChangeRole: {
			string target = roleOf(list_id, a.user_id);
			if (target.empty()) {
				reason = "not a member";
			} else if (target == "owner") {
				reason = "cannot change the owner's role";
			} else if (!isAssignableRole(a.new_value)) {
				reason = "invalid role";
			} else if (target == a.new_value) {
				reason = "no change";
			} else if (exec("update sharedlist_members set role = ? where sharedlist_id = ? and user_id = ?",
					{a.new_value, list_id, a.user_id})) {
				a.old_value = target;
			}
			break;
		}
		}
		if (fail) break;

		string status = reason.empty() ? "applied" : "rejected";
		long long id = logAction(db, list_id, actor_id, a, out.batch_id, status, reason);
		if (id < 0) { fail = true; break; }
		if (out.batch_id == 0) {
			// a batch is named after its first action
			out.batch_id = id;
			if (!exec("update sharedlist_actions set batch_id = id where id = ?", {static_cast<int64_t>(id)})) break;
		}
		out.results.push_back({id, a.kind, status, reason});
	}

	if (!fail && dirty) {
		exec("delete from sharedlist_tracks where sharedlist_id = ?", {list_id});
		for (size_t i = 0; i < order.size() && !fail; i++) {
			exec("insert into sharedlist_tracks (sharedlist_id, origin_id, position) values (?, ?, ?)",
				{list_id, order[i], static_cast<int>(i)});
		}
	}

	if (fail) {
		db->execute("ROLLBACK");
		out.results.clear();
		out.http = 500;
		out.error = "db error";
		return out;
	}
	db->execute("COMMIT");
	out.ok = true;
	return out;
}
