#include <iostream>
#include <cstdlib>
#include <openssl/evp.h>
#include "include/user.hpp"
#include "include/util.hpp"
#include "include/crow.h"

using namespace std;

int main(int argc, char **argv) {

	crow::SimpleApp app;
	
	CROW_ROUTE(app, "/")([]() {
		crow::response res;
		res.set_header("Access-Control-Allow-Origin", "*");
		res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
		res.set_header("Content-Type", "application/json");

		res.write("{\"message\": \"Hello, world!\"}");
        	return res;
	});

	CROW_ROUTE(app, "/getUser").methods("POST"_method)
	([](const crow::request& req) {
		auto body = crow::json::load(req.body);
		string username = body["username"].s();
		User *u = new User();
		vector<UserModel> users = u->getUserName(username);
		crow::json::wvalue user_info;
		if (users[0].id == -1) {
			user_info["error"] = "There was an error with the data.";
		}
		if (users.empty()) {
			user_info["error"] = "No users found";
		} else {
			user_info["id"] = users[0].id;
			user_info["user"] = users[0].username;
		}
		delete u;
		return user_info;
	});

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
		crow::json::wvalue res;
		const string ip = req.get_header_value("X-Forwarded-For");
		const string user = req.url_params.get("user") ? req.url_params.get("user") : "!error!";
		vector<UserModel> users = util->getUserFromUsername(user);
		vector<SessionModel> sessions = util->getSessionFromUsername(user);
		int session = util->hasValidSession(users[0].id, ip, sessions[0].session_file, user);
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
			// todo: store state somewhere to be checked on return
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
	 	Util *util = new Util();
	 	crow::json::wvalue res;
		// get code and state from query params, request token. store token in session file on server?
		string state = req.url_params.get("state") ? req.url_params.get("state") : "!error!";
        	string code = req.url_params.get("code") ? req.url_params.get("code") : "!error!";
		string url = "https://sharedlist.us/api/sso_callback";
		const char* client_id = getenv("SPOTIFY_CLIENT_ID");
		const char* client_secret = getenv("SPOTIFY_CLIENT_SECRET");
		// if valid state
		string post_data = "code=" + code + "&redirect_uri=" + url + "&grant_type=authorization_code";
		string response = util->make_http_request("https://accounts.spotify.com/api/token", "POST", post_data, client_id, client_secret);
		crow::response redirect;
		redirect.code = 302; 
		redirect.add_header("Location", "/app.html");
        	redirect.write(response);  // Write the HTTP response content
		return redirect;
	});

	app.port(18808).multithreaded().run();
}
