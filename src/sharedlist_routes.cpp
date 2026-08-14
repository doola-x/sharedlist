#include <iostream>
#include <thread>
#include "include/sharedlist_routes.hpp"

using namespace std;

void registerSharedlistRoutes(crow::SimpleApp& app, Sharedlist& sharedlist, User& user) {

	CROW_ROUTE(app, "/sharedlist").methods("GET"_method)
	([&sharedlist](const crow::request& req) {
		if (!req.url_params.get("sharedlist_id") || !req.url_params.get("offset") || !req.url_params.get("limit")) {
			crow::json::wvalue err;
			err["error"] = "missing params";
			return crow::response(400, err);
		}

		int sharedlist_id = stoi(req.url_params.get("sharedlist_id"));
		int offset = stoi(req.url_params.get("offset"));
		int limit = stoi(req.url_params.get("limit"));

		auto tracks = sharedlist.getSharedlistTracks(sharedlist_id, offset, limit);

		crow::json::wvalue res;
		vector<crow::json::wvalue> items;
		for (auto& t : tracks) {
			crow::json::wvalue item;
			item["name"] = t.name;
			item["artists"] = t.artists;
			item["album"] = t.album;
			item["spotify_id"] = t.spotify_id;
			items.push_back(move(item));
		}
		res["has_next"] = items.size() == limit + 1 ? true : false;
		res["items"]  = crow::json::wvalue(move(items));
		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/sharedlist").methods("POST"_method)
	([&user, &sharedlist](const crow::request& req) {
		auto body = crow::json::load(req.body);
		string username = body["username"].s();
		string origin_type = body["origin_type"].s();
		string origin_id = body["origin_id"].s();
		crow::json::wvalue res;

		vector<UserModel> users = user.getUser(username);
		if (users.size() != 1) {
			res["status"] = "something went wrong fetching user information";
			return crow::response(400, res);
		}

		int sharedlist_id = sharedlist.createSharedlist(users[0].id, origin_type, origin_id);
		if (sharedlist_id == -1) {
			res["status"] = "failure";
			return crow::response(400, res);
		}
		
		cout << "running threads...\n";
		string access_token = user.fetchToken(users[0].id);
		thread([&sharedlist, access_token, origin_id, sharedlist_id]() {
			auto tracks = sharedlist.fetchSpotifyTracks(access_token, origin_id);
			if (sharedlist.addSharedlistTracks(tracks) == -1) {
				cerr << "aborting sync for sharedlist " << sharedlist_id << endl;
				return;
			}
			sharedlist.syncSharedlistTracks(tracks, sharedlist_id);
		}).detach();

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
