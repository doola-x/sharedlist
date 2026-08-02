#include <iostream>
#include <cstdlib>
#include "include/user_routes.hpp"

using namespace std;

void registerUserRoutes(crow::SimpleApp& app, User& user, SessionManager& session, Crypto& crypto, HttpClient& http) {
	
	CROW_ROUTE(app, "/signup").methods("POST"_method)
	([&user, &crypto](const crow::request& req) {
		crow::json::wvalue res;
		auto body = crow::json::load(req.body);

		string username = body["username"].s();
		string password = body["password"].s();

		if (username.empty() || password.empty()) {
			res["status"] = "error";
			res["msg"] = "the username or password was empty.";
			return crow::response(400, res);
		}

		PassComponents pc = crypto.hashPassword(password);
		int result = user.signupUser(username, pc.hashword, pc.salt);
		if (result == -1) {
			res["status"] = "error";
			res["msg"] = "the username or password was empty.";
			return crow::response(400, res);
		}
		res["status"] = "success";
		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/signin").methods("POST"_method)
	([&user, &session](const crow::request& req) {
		crow::json::wvalue res;
		auto body = crow::json::load(req.body);
		string username = body["username"].s();
		string password = body["password"].s();

		if (username.empty() || password.empty()) {
			res["status"] = "error";
			res["msg"] = "the username or password was empty.";
			return crow::response(400, res);
		}
		int result = user.loginUser(username, password);
		if (result == 0) {
			int sess = session.createSession(username, req.get_header_value("X-Forwarded-For"));
			if (sess) {
				res["status"] = "failure";
				return crow::response(400, res);
			}
			res["status"] = "success";
			return crow::response(200, res);
		} else {
			res["status"] = "failed to login user";
			return crow::response(400, res);
		}
	});

	CROW_ROUTE(app, "/spotify_signin").methods("GET"_method)
	([&user, &session, &crypto](const crow::request& req) {
		crow::json::wvalue res;
		const string ip = req.get_header_value("X-Forwarded-For");
		const string user_s = req.url_params.get("user") ? req.url_params.get("user") : "!error!";
		vector<UserModel> users = user.getUser(user_s);
		vector<SessionModel> sessions = session.getSessionFromUsername(user_s);
		int valid = session.hasValidSession(users[0].id, ip, sessions[0].session_file, user_s);
		if (valid) {
			res["status"] = "failure";
			return crow::response(400, res);
		}

		const char* client_id = getenv("SPOTIFY_CLIENT_ID");
		const char* client_secret = getenv("SPOTIFY_CLIENT_SECRET");
		if (client_id && client_secret) {
			string url = "https://sharedlist.us/api/sso_callback";
			string state = crypto.generateSalt(16);
			user.recordState(user_s, state);
			string scope = "playlist-modify-private playlist-read-private user-read-currently-playing";
			string req_url = "https://accounts.spotify.com/authorize?";
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
	([&user, &http](const crow::request& req) {
		crow::json::wvalue res;
		string state = req.url_params.get("state") ? req.url_params.get("state") : "!error!";
		string code = req.url_params.get("code") ? req.url_params.get("code") : "!error!";

		SpotifyStateModel state_obj = user.fetchState(state);
		if (state != state_obj.state) {
			res["status"] = "failure";
			return crow::response(400, res);
		}
		string url = "https://sharedlist.us/api/sso_callback";
		const char* client_id = getenv("SPOTIFY_CLIENT_ID");
		const char* client_secret = getenv("SPOTIFY_CLIENT_SECRET");
		string post_data = "code=" + code + "&redirect_uri=" + url + "&grant_type=authorization_code";
		string response = http.request("https://accounts.spotify.com/api/token", "POST", post_data, client_id, client_secret);
		crow::json::rvalue token = crow::json::load(response);

		int updated = user.recordToken(state_obj.user_id, state_obj.state, token["access_token"].s());
		if (updated == -1) {
			res["status"] = "failure";
			return crow::response(400, res);
		}
		crow::response redirect;
		redirect.code = 302;
		redirect.add_header("Location", "/app.html?id_token=true");
		return redirect;
	});

	CROW_ROUTE(app, "/spotify_playlists").methods("POST"_method)
	([&user, &http](const crow::request& req) {
		auto body = crow::json::load(req.body);
		string username = body["username"].s();

		vector<UserModel> users = user.getUser(username);
		string access_token = user.fetchToken(users[0].id);
		string response = http.request("https://api.spotify.com/v1/me/playlists?limit=12", "GET", "", "", "", access_token);

		crow::json::wvalue res;
		bool done = false;

		auto playlists = crow::json::load(response);
		crow::json::rvalue next = playlists["next"];
		crow::json::rvalue items = playlists["items"];
		auto items_vec = items.lo();

		while (!done) {
			response = http.request(next.s(), "GET", "", "", "", access_token);
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

	CROW_ROUTE(app, "/spotify_track").methods("POST"_method)
	([&user, &http](const crow::request& req) {
		auto body = crow::json::load(req.body);
		string username = body["username"].s();
		crow::json::wvalue res;

		vector<UserModel> users = user.getUser(username);
		if (size_t u_size = users.size(); u_size == 0 || u_size > 1) {
			cerr << "something went wrong fetching user data for user "
				<< username << endl;
			res["status"] = "failure";
			return crow::response(400, res);
		}
		string access_token = user.fetchToken(users[0].id);
			

	});
}
