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

	CROW_ROUTE(app, "/getUser")([]() {
		User *u = new User();
		vector<UserModel> users = u->getUserName(0);
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
	 	auto x = crow::json::load(req.body);
		string username = x["username"].s();
		string password = x["password"].s();

		if (username.empty() || password.empty()) {
			return crow::response(400, "username and password are required");
		}

		User *user = new User();
		Util *util = new Util();
		
		string hashword = util->hashPassword(password);		
		if (hashword.empty()) {
			return crow::response(400, "util fns down");
		}
		int result = user->signupUser(username, hashword);
		if (result == -1) {
			return crow::response(400, "response was bad");
		}
		return crow::response(200, "success");		
	});

	app.port(18808).multithreaded().run();
}
