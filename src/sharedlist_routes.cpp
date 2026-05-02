#include <iostream>
#include <thread>
#include "include/sharedlist_routes.hpp"

using namespace std;

void registerSharedlistRoutes(crow::SimpleApp& app, Sharedlist& sharedlist, User& user) {

	CROW_ROUTE(app, "/sharedlist").methods("GET"_method)
	([&user](const crow::request& req) {
		string username = req.url_params.get("user") ? req.url_params.get("user") : "!error!";
		string sp_id = req.url_params.get("sp_id") ? req.url_params.get("sp_id") : "!error!";
		string sp = req.url_params.get("sp") ? req.url_params.get("sp") : "!error!";

		if (username == "!error!" || sp_id == "!error!" || sp == "!error") {
			// missing params err
		}

		vector<UserModel> users = user.getUser(username);

		crow::json::wvalue res;
		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/sharedlist").methods("POST"_method)
	([&user, &sharedlist](const crow::request& req) {
		auto body = crow::json::load(req.body);
		string username = body["username"].s();
		string origin_type = body["origin_type"].s();
		string origin_id = body["origin_id"].s();

		vector<UserModel> users = user.getUser(username);
		int sharedlist_id = sharedlist.createSharedlist(users[0].id, origin_type, origin_id);

		crow::json::wvalue res;
		if (sharedlist_id == -1) {
			res["status"] = "failure";
			return crow::response(400, res);
		}

		string access_token = user.fetchToken(users[0].id);
		thread([&sharedlist, access_token, origin_id, sharedlist_id]() {
			cout << "running thread" << endl;
			sharedlist.addSharedlistTracks(access_token, origin_id);
			cout << "added tracks" << endl;
			sharedlist.syncSharedlistTracks(access_token, origin_id, sharedlist_id);
		}).detach();

		res["status"] = "success";
		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/test").methods("GET"_method)
	([](const crow::request& req) {
		crow::json::wvalue ret;
		return crow::response(200, ret);
	});
}
