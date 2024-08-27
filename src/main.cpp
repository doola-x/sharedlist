#include <iostream>
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
			res["status"] = "success";
			return crow::response(200, res);
		} else {
			res["status"] = "error";
			return crow::response(400, res);
		}
	});

	app.port(18808).multithreaded().run();
}
