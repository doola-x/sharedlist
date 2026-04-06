#include <iostream>
#include <cstdlib>
#include <openssl/evp.h>
#include "include/user.hpp"
#include "include/util.hpp"
#include "include/sharedlist.hpp"
#include "include/crow.h"

using namespace std;

int main(int argc, char **argv) {

	crow::SimpleApp app;
	
	CROW_ROUTE(app, "/signup").methods("POST"_method)
	([](const crow::request& req) {
	 	crow::json::wvalue res;
	 	auto body = crow::json::load(req.body);
		
		string username = body["username"].s();
		string password = body["password"].s();

		if (username.empty() || password.empty()) {
			res["status"] = "error";
			res["msg"] = "the username or password was empty.";
			return crow::response(400, res);
		}

		User *user = new User();
		Util *util = new Util();
		
		PassComponents pc = util->hashPassword(password);		
		int result = user->signupUser(username, pc.hashword, pc.salt);
		if (result == -1) {
			res["status"] = "error";
			res["msg"] = "the username or password was empty.";
			return crow::response(400, res);
		}
		res["status"] = "success";
		return crow::response(200, res);		
	});

	CROW_ROUTE(app, "/signin").methods("POST"_method)
	([](const crow::request& req) {
		crow::json::wvalue res;
		auto body = crow::json::load(req.body);
		auto ip = req.get_header_value("X-Forwarded-For");
		string username = body["username"].s();
		string password = body["password"].s();

		if (username.empty() || password.empty()) {
			res["status"] = "error";
			res["msg"] = "the username or password was empty.";
			return crow::response(400, res);
		}
		User *user = new User();
		Util *util = new Util();
		int result = user->loginUser(username, password);
		if (result == 0) {
			int session = util->createSession(username, req.get_header_value("X-Forwarded-For"));
			if (session) {
				res["status"] = "failure";
				return crow::response(400, res);
			}
			res["status"] = "success";
			return crow::response(200, res);
		} else {
			res["status"] = "error";
			return crow::response(400, res);
		}
	});

	CROW_ROUTE(app, "/spotify_signin").methods("GET"_method)
	([](const crow::request& req) {
		Util *util = new Util();
		User *user = new User();
		crow::json::wvalue res;
		const string ip = req.get_header_value("X-Forwarded-For");
		const string user_s = req.url_params.get("user") ? req.url_params.get("user") : "!error!";
		vector<UserModel> users = util->getUser(user_s);
		vector<SessionModel> sessions = util->getSessionFromUsername(user_s);
		int session = util->hasValidSession(users[0].id, ip, sessions[0].session_file, user_s);
		if (session) {
			res["status"] = "failure";
			return crow::response(400, res);
		}

		auto body = crow::json::load(req.body);
		const char* client_id = getenv("SPOTIFY_CLIENT_ID");
		const char* client_secret = getenv("SPOTIFY_CLIENT_SECRET");
		if (client_id && client_secret) {
			string url = "https://sharedlist.us/api/sso_callback";
			string state = util->generateSalt(16);
			int saved = user->recordState(user_s, state);
			string req_url = "https://accounts.spotify.com/authorize?";
			string scope = "playlist-modify-private playlist-read-private user-read-currently-playing";
			req_url += "response_type=code&client_id=" + string(client_id) + "&scope=" + scope + "&redirect_uri=" + url + "&state=" + state;
			crow::response redirect;
			redirect.code = 302; 
			redirect.add_header("Location", req_url); 
			return redirect;
		} else {
			cerr << "Environment variables not set" << endl;
		}
		return crow::response(400, res);
	});

	CROW_ROUTE(app, "/sso_callback").methods("GET"_method)
	([](const crow::request& req) {
	 	User *user = new User();
		Util *util = new Util();
	 	crow::json::wvalue res;
		string state = req.url_params.get("state") ? req.url_params.get("state") : "!error!";
        	string code = req.url_params.get("code") ? req.url_params.get("code") : "!error!";

		SpotifyStateModel state_obj = user->fetchState(state);
		if (state != state_obj.state) {
			crow::json::wvalue res;
			res["status"] = "failure";
			return crow::response(400, res);
		}
		string url = "https://sharedlist.us/api/sso_callback";
		const char* client_id = getenv("SPOTIFY_CLIENT_ID");
		const char* client_secret = getenv("SPOTIFY_CLIENT_SECRET");
		string post_data = "code=" + code + "&redirect_uri=" + url + "&grant_type=authorization_code";
		string response = util->make_http_request("https://accounts.spotify.com/api/token", "POST", post_data, client_id, client_secret);
		crow::json::rvalue token = crow::json::load(response);

		int updated = user->recordToken(state_obj.user_id, state_obj.state, token["access_token"].s());
		if (updated == -1) {
			crow::json::wvalue res;
			res["status"] = "failure";
			return crow::response(400, res);
		}
		crow::response redirect;
		redirect.code = 302; 
		redirect.add_header("Location", "/app.html?id_token=true");
		return redirect;
	});

	CROW_ROUTE(app, "/spotify_playlists").methods("POST"_method)
	([](const crow::request& req) {
		auto body = crow::json::load(req.body);
		string username = body["username"].s();
		User *user = new User();
		Util *util = new Util();

		vector<UserModel> users = util->getUser(username);
		string access_token = user->fetchToken(users[0].id);
		string response = util->make_http_request("https://api.spotify.com/v1/me/playlists?limit=12", "GET", "", "", "", access_token);

		crow::json::wvalue res;
		bool done = false;

		auto playlists = crow::json::load(response);
		crow::json::rvalue next = playlists["next"];
		crow::json::rvalue items = playlists["items"];
		auto items_vec = items.lo();

		while (!done) {
			response = util->make_http_request(next.s(), "GET", "", "", "", access_token);
			playlists = crow::json::load(response);
			if (auto val = playlists["next"]; val.t() == crow::json::type::Null) {
				done = true;
			} else {
				next = playlists["next"];
			}
			crow::json::rvalue items_new = playlists["items"];
			for (auto item : items_new.lo()) {
				items_vec.push_back(item);
			}
		}

		res["status"] = "success";
		res["items"] = items_vec;
		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/sharedlist").methods("GET"_method)
	([](const crow::request& req) {
		string username = req.url_params.get("user") ? req.url_params.get("user") : "!error!";
		string sp_id = req.url_params.get("sp_id") ? req.url_params.get("sp_id") : "!error!";
		string sp = req.url_params.get("sp") ? req.url_params.get("sp") : "!error!";

		if (username == "!error!" || sp_id == "!error!" || sp == "!error") {
			// missing params err
		}

		User *user = new User();
		Util *util = new Util();

		vector<UserModel> users = util->getUser(username);
				
		crow::json::wvalue res;
		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/sharedlist").methods("POST"_method)
	([](const crow::request& req) {
		auto body = crow::json::load(req.body);
		string username = body["username"].s();
		string origin_type = body["origin_type"].s();
		string origin_id = body["origin_id"].s();

		User *user = new User();
		Util *util = new Util();
		Sharedlist *sharedlist = new Sharedlist();

		vector<UserModel> users = util->getUser(username);
		int result = sharedlist->createSharedlist(users[0].id, origin_type, origin_id);

		crow::json::wvalue res;
		if (result == -1) {
			res["status"] = "failure";
			return crow::response(400, res);
		}
		res["status"] = "success";

		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/test").methods("GET"_method)
	([](const crow::request& req) {
		auto body = crow::json::load(req.body);
		string username = body["username"].s();
		string origin_id = body["origin_id"].s();

		User *user = new User();
		Util *util = new Util();
		Sharedlist *sharedlist = new Sharedlist();

		vector<UserModel> users = util->getUser(username);
		string access_token = user->fetchToken(users[0].id);
		cout << access_token << endl;
		crow::json::rvalue res = sharedlist->syncSharedlist(access_token, origin_id);

		crow::json::wvalue ret;
		ret["res"] = res;
		return crow::response(200, ret);
	});

	app.port(18808).multithreaded().run();
}
