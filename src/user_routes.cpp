#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include "include/user_routes.hpp"
#include "data_types.hpp"

using namespace std;

void registerUserRoutes(crow::App<ScopedRequest>& app) {
	auto& middleware = app.get_middleware<ScopedRequest>();
	CROW_ROUTE(app, "/signup").methods("POST"_method)
	([&](const crow::request& req) {
		crow::json::wvalue res;
		auto body = crow::json::load(req.body);
        std::cout << "fetching context..." << std::endl;
        auto& ctx = app.get_context<ScopedRequest>(req);
        
        if (!ctx.user_->db) {
            throw std::runtime_error("user db is nullptr!");
        }

		string username = body["username"].s();
		string password = body["password"].s();

		if (username.empty() || password.empty()) {
			res["status"] = "error";
			res["msg"] = "the username or password was empty.";
			return crow::response(400, res);
		}

		PassComponents pc = middleware.crypto->hashPassword(password);
		int result = ctx.user_->signupUser(username, pc.hashword, pc.salt);
		if (result == -1) {
			res["status"] = "error";
			return crow::response(400, res);
		}
		res["status"] = "success";
		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/signin").methods("POST"_method)
	([&app](const crow::request& req) {
		crow::json::wvalue res;
		const auto& body = crow::json::load(req.body);
        auto& ctx = app.get_context<ScopedRequest>(req); 

		string username = body["username"].s();
		string password = body["password"].s();
		if (username.empty() || password.empty()) {
			res["status"] = "error";
			res["msg"] = "the username or password was empty.";
			return crow::response(400, res);
		}

		int result = ctx.user_->loginUser(username, password);
		if (result == -1) {
			res["status"] = "failed to login user";
			return crow::response(400, res);	
		}

		string token = ctx.session_->createSession(username, req.get_header_value("X-Forwarded-For"));
		if (token == "") {
			res["status"] = "failure";
			return crow::response(400, res);
		}

		string cookieHeader = "session_token=" + token + 
                                   "; Path=/" +
                                   "; HttpOnly" + 
                                   "; Secure" + 
                                   "; SameSite=Lax";
		res["status"] = "success";
		return crow::response(200, res);
	});

	CROW_ROUTE(app, "/spotify_signin").methods("GET"_method)
	([&](const crow::request& req) {
		crow::json::wvalue res;
		const string& ip = req.get_header_value("X-Forwarded-For");
		const string& user_s = req.url_params.get("user") ? req.url_params.get("user") : "!error!";
        auto& ctx = app.get_context<ScopedRequest>(req);

		UserModel user = ctx.user_->getUser(user_s);
		SessionModel session = ctx.session_->getSessionFromUsername(user.username);
		int valid = ctx.session_->hasValidSession(user.id, ip, session.session_token, user_s);
		if (valid == -1) {
			res["status"] = "failure";
			return crow::response(400, res);
		}

		const char* client_id = getenv("SPOTIFY_CLIENT_ID");
		const char* client_secret = getenv("SPOTIFY_CLIENT_SECRET");
		if (client_id && client_secret) {
			string url = "https://sharedlist.us/api/sso_callback";
			string state = middleware.crypto->generateSalt(16);
			if (ctx.user_->recordState(user_s, state) != 0) {
				res["status"] = "failure";
				return crow::response(400, res);
			}
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
	([&](const crow::request& req) {
		crow::json::wvalue res;
		const string& state = req.url_params.get("state") ? req.url_params.get("state") : "!error!";
		const string& code = req.url_params.get("code") ? req.url_params.get("code") : "!error!";
        const auto& ctx = app.get_context<ScopedRequest>(req); 


		SpotifyStateModel state_obj = ctx.user_->fetchState(state);
		if (state != state_obj.state) {
			res["status"] = "failure";
			return crow::response(400, res);
		}
		string url = "https://sharedlist.us/api/sso_callback";
		const char* client_id = getenv("SPOTIFY_CLIENT_ID");
		const char* client_secret = getenv("SPOTIFY_CLIENT_SECRET");
		string post_data = "code=" + code + "&redirect_uri=" + url + "&grant_type=authorization_code";
		string response = middleware.http->request("https://accounts.spotify.com/api/token", "POST", post_data, client_id, client_secret);
		crow::json::rvalue token = crow::json::load(response);

		int updated = ctx.user_->recordToken(state_obj.user_id, state_obj.state, token["access_token"].s());
		if (updated == 1) {
			res["status"] = "failure";
			return crow::response(400, res);
		}
		crow::response redirect;
		redirect.code = 302;
		redirect.add_header("Location", "/app.html?id_token=true");
		return redirect;
	});

	CROW_ROUTE(app, "/spotify_playlists").methods("POST"_method)
	([&](const crow::request& req) {
		crow::json::wvalue res;
		const auto& body = crow::json::load(req.body);
        const auto& ctx = app.get_context<ScopedRequest>(req);
        const string& username = body["username"].s();
	
		auto user = ctx.user_->getUser(username);
		auto token_obj = ctx.user_->fetchToken(user.id);
		string response = middleware.http->request("https://api.spotify.com/v1/me/playlists?limit=12", "GET", "", "", "", token_obj.access_token);

		bool done = false;
		auto playlists = crow::json::load(response);
		crow::json::rvalue next = playlists["next"];
		crow::json::rvalue items = playlists["items"];
		auto items_vec = items.lo();

		while (!done) {
			response = middleware.http->request(next.s(), "GET", "", "", "", token_obj.access_token);
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
	([&](const crow::request& req) {
		crow::json::wvalue res;
		const auto& body = crow::json::load(req.body);
        const auto& ctx = app.get_context<ScopedRequest>(req);
		const string& username = body["username"].s();

        auto user = ctx.user_->getUser(username);
		auto access_token = ctx.user_->fetchToken(user.id);
		res["status"] = "success";
		return crow::response(200, res);
	});
}
