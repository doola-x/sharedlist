#include <iostream>
#include <thread>
#include "include/sharedlist_routes.hpp"
#include "data_types.hpp"

using namespace std;

static crow::json::wvalue memberJson(const MemberModel& m) {
	crow::json::wvalue j;
	j["user_id"] = m.user_id;
	j["username"] = m.username;
	j["role"] = m.role;
	j["added_at"] = m.added_at;
	return j;
}

static crow::response applyResponse(const Sharedlist::ApplyResult& r) {
	crow::json::wvalue res;
	if (!r.ok) {
		res["error"] = r.error;
		return crow::response(r.http, res);
	}
	vector<crow::json::wvalue> results;
	for (auto& a : r.results) {
		crow::json::wvalue item;
		item["id"] = a.id;
		item["kind"] = kindName(a.kind);
		item["status"] = a.status;
		item["reason"] = a.reason;
		results.push_back(std::move(item));
	}
	res["status"] = "success";
	res["batch_id"] = r.batch_id;
	res["results"] = crow::json::wvalue(std::move(results));
	return crow::response(200, res);
}

static crow::response errorResponse(int code, const string& msg) {
	crow::json::wvalue err;
	err["error"] = msg;
	return crow::response(code, err);
}

void registerSharedlistRoutes(crow::App<ScopedRequest>& app) {

	// GET  ?since=<action id>&limit=  -> the log, oldest first
	// POST {"actions":[{"kind":"add_track","track_id":"..","position":0}, ...]} -> one batch
	CROW_ROUTE(app, "/sharedlist/<int>/actions").methods("GET"_method, "POST"_method)
	([&](const crow::request& req, int sharedlist_id) {
		auto& ctx = app.get_context<ScopedRequest>(req);
		if (ctx.user_id < 0) return errorResponse(401, "not signed in");

		if (req.method == "GET"_method) {
			if (ctx.sharedlist_->roleOf(sharedlist_id, ctx.user_id).empty()) return errorResponse(403, "forbidden");
			long long since = req.url_params.get("since") ? stoll(req.url_params.get("since")) : 0;
			int limit = req.url_params.get("limit") ? stoi(req.url_params.get("limit")) : 100;
			limit = max(1, min(limit, 500));

			vector<crow::json::wvalue> items;
			for (auto& r : ctx.sharedlist_->history(sharedlist_id, since, limit)) {
				crow::json::wvalue item;
				item["id"] = r.id;
				item["batch_id"] = r.batch_id;
				item["actor"] = r.actor;
				item["kind"] = r.kind;
				item["payload"] = crow::json::wvalue(crow::json::load(r.payload));
				item["status"] = r.status;
				item["reason"] = r.reason;
				item["created_at"] = r.created_at;
				items.push_back(std::move(item));
			}
			crow::json::wvalue res;
			res["items"] = crow::json::wvalue(std::move(items));
			return crow::response(200, res);
		}

		auto body = crow::json::load(req.body);
		if (!body || !body.has("actions") || body["actions"].t() != crow::json::type::List) {
			return errorResponse(400, "expected {\"actions\": [...]}");
		}
		vector<Action> actions;
		for (auto& entry : body["actions"]) {
			auto a = Action::fromKindAndJson(jsonString(entry, "kind"), entry);
			if (!a) return errorResponse(400, "bad action: " + jsonString(entry, "kind"));
			actions.push_back(std::move(*a));
		}
		return applyResponse(ctx.sharedlist_->apply(sharedlist_id, ctx.user_id, std::move(actions)));
	});

	// membership. owner adds anyone by username; thin wrappers over add_member /
	// change_role / remove_member so they land in the same log.
	//   GET                              -> members
	//   POST   {"username","role"}       -> add_member    (owner)
	//   PATCH  {"username","role"}       -> change_role   (owner)
	//   DELETE {"username"}              -> remove_member (owner, or yourself to leave)
	CROW_ROUTE(app, "/sharedlist/<int>/members").methods("GET"_method, "POST"_method, "PATCH"_method, "DELETE"_method)
	([&](const crow::request& req, int sharedlist_id) {
		auto& ctx = app.get_context<ScopedRequest>(req);
		if (ctx.user_id < 0) return errorResponse(401, "not signed in");

		if (req.method == "GET"_method) {
			if (ctx.sharedlist_->roleOf(sharedlist_id, ctx.user_id).empty()) return errorResponse(403, "forbidden");
			vector<crow::json::wvalue> members;
			for (auto& m : ctx.sharedlist_->getMembers(sharedlist_id)) members.push_back(memberJson(m));
			crow::json::wvalue res;
			res["members"] = crow::json::wvalue(std::move(members));
			return crow::response(200, res);
		}

		auto body = crow::json::load(req.body);
		string username = body ? jsonString(body, "username") : "";
		if (username.empty()) return errorResponse(400, "missing username");

		int target_id = ctx.sharedlist_->findUserId(username);
		if (target_id < 0) return errorResponse(404, "no such user");

		Action a;
		a.user_id = target_id;
		if (req.method == "DELETE"_method) {
			a.kind = ActionKind::RemoveMember;
		} else {
			a.kind = req.method == "POST"_method ? ActionKind::AddMember : ActionKind::ChangeRole;
			a.new_value = jsonString(body, "role");
		}

		auto result = ctx.sharedlist_->apply(sharedlist_id, ctx.user_id, {a});
		// a single-action batch that was rejected is a client error, not a 200
		if (result.ok && result.results.size() == 1 && result.results[0].status == "rejected") {
			return errorResponse(409, result.results[0].reason);
		}
		return applyResponse(result);
	});

	CROW_ROUTE(app, "/sharedlist").methods("GET"_method)
	([&](const crow::request& req) {
		if (!req.url_params.get("sharedlist_id") || !req.url_params.get("offset") || !req.url_params.get("limit")) {
			crow::json::wvalue err;
			err["error"] = "missing params";
			return crow::response(400, err);
		}
        
        auto& ctx = app.get_context<ScopedRequest>(req);
		if (ctx.user_id < 0) {
			crow::json::wvalue err;
			err["error"] = "not signed in";
			return crow::response(401, err);
		}

		int sharedlist_id = stoi(req.url_params.get("sharedlist_id"));
		if (ctx.sharedlist_->roleOf(sharedlist_id, ctx.user_id).empty()) {
			crow::json::wvalue err;
			err["error"] = "forbidden";
			return crow::response(403, err);
		}
		int offset = stoi(req.url_params.get("offset"));
		int limit = stoi(req.url_params.get("limit"));

		auto tracks = ctx.sharedlist_->getSharedlistTracks(sharedlist_id, offset, limit);

		crow::json::wvalue res;
		vector<crow::json::wvalue> items;
		for (auto& t : tracks) {
			crow::json::wvalue item;
			item["name"] = t.name;
			item["artists"] = t.artists;
			item["album"] = t.album;
			item["spotify_id"] = t.spotify_id;
			item["duration_ms"] = t.duration_ms;
			item["image_url"] = t.image_url;
			item["explicit"] = t.explicit_;
			items.push_back(std::move(item));
		}
		res["has_next"] = items.size() == limit + 1 ? true : false;
		res["items"]  = crow::json::wvalue(std::move(items));
		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/sharedlist_info").methods("GET"_method)
	([&](const crow::request& req) {
		crow::json::wvalue res;
		if (!req.url_params.get("sharedlist_id")) {
			res["error"] = "missing params";
			return crow::response(400, res);
		}

		auto& ctx = app.get_context<ScopedRequest>(req);
		if (ctx.user_id < 0) {
			res["error"] = "not signed in";
			return crow::response(401, res);
		}

		int sharedlist_id = stoi(req.url_params.get("sharedlist_id"));
		string role = ctx.sharedlist_->roleOf(sharedlist_id, ctx.user_id);
		if (role.empty()) {
			res["error"] = "forbidden";
			return crow::response(403, res);
		}

		SharedlistInfoModel info;
		if (!ctx.sharedlist_->getSharedlistInfo(sharedlist_id, info)) {
			res["error"] = "not found";
			return crow::response(404, res);
		}

		// sharedlists created before metadata caching: backfill from the origin once
		if (info.name.empty() && info.origin_type == "spotify") {
			string access_token = ctx.user_->getValidAccessToken(ctx.user_id, *app.get_middleware<ScopedRequest>().http);
			SharedlistMetaModel meta;
			if (!access_token.empty()
				&& ctx.sharedlist_->fetchSpotifyPlaylistMeta(access_token, info.origin_id, meta)
				&& ctx.sharedlist_->saveSharedlistMeta(sharedlist_id, meta) == 0) {
				ctx.sharedlist_->getSharedlistInfo(sharedlist_id, info);
			}
		}

		res["owner"] = info.owner;
		res["track_count"] = info.track_count;
		res["total_ms"] = info.total_ms;
		res["created_at"] = info.created_at;
		res["origin_type"] = info.origin_type;
		res["name"] = info.name;
		res["description"] = info.description;
		res["image_url"] = info.image_url;
		res["origin_owner"] = info.origin_owner;
		res["role"] = role;
		vector<crow::json::wvalue> members;
		for (auto& m : ctx.sharedlist_->getMembers(sharedlist_id)) members.push_back(memberJson(m));
		res["members"] = crow::json::wvalue(std::move(members));
		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/sharedlist").methods("POST"_method)
	([&](const crow::request& req) {
		auto body = crow::json::load(req.body);
		auto& ctx = app.get_context<ScopedRequest>(req);
		if (ctx.user_id < 0) {
			crow::json::wvalue err;
			err["status"] = "not signed in";
			return crow::response(401, err);
		}
		string origin_type = body["origin_type"].s();
		string origin_id = body["origin_id"].s();
		crow::json::wvalue res;

		UserModel fetched_user = ctx.user_->getUserById(ctx.user_id);

		int sharedlist_id = ctx.sharedlist_->createSharedlist(fetched_user.id, origin_type, origin_id);
		if (sharedlist_id == -1) {
			res["status"] = "failure";
			return crow::response(400, res);
		}

		string access_token = ctx.user_->getValidAccessToken(fetched_user.id, *app.get_middleware<ScopedRequest>().http);
		if (access_token.empty()) {
			res["status"] = "failure";
			return crow::response(401, res);
		}
		auto tracks = ctx.sharedlist_->fetchSpotifyTracks(access_token, origin_id);
		if (ctx.sharedlist_->addSharedlistTracks(tracks) == -1) {
            res["status"] = "failure";
			return crow::response(400, res);

		}
		ctx.sharedlist_->syncSharedlistTracks(tracks, sharedlist_id, fetched_user.id);

		// best effort: cache name / cover / owner now so page loads never need the origin
		SharedlistMetaModel meta;
		if (ctx.sharedlist_->fetchSpotifyPlaylistMeta(access_token, origin_id, meta)) {
			ctx.sharedlist_->saveSharedlistMeta(sharedlist_id, meta);
		}

		res["status"] = "success";
		res["sharedlist_id"] = sharedlist_id;
		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/test").methods("GET"_method)
	([](const crow::request& req) {
		crow::json::wvalue ret;
		return crow::response(200, ret);
	});
}
