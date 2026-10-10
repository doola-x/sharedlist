#include <iostream>
#include <thread>
#include "include/sharedlist_routes.hpp"
#include "data_types.hpp"

using namespace std;

void registerSharedlistRoutes(crow::App<ScopedRequest>& app) {

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
		if (!ctx.sharedlist_->isOwner(sharedlist_id, ctx.user_id)) {
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
			items.push_back(std::move(item));
		}
		res["has_next"] = items.size() == limit + 1 ? true : false;
		res["items"]  = crow::json::wvalue(std::move(items));
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
		ctx.sharedlist_->syncSharedlistTracks(tracks, sharedlist_id);

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
