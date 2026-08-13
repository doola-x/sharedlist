#include <iostream>
#include "include/dal.hpp"
#include "include/crypto.hpp"
#include "include/http_client.hpp"
#include "include/session.hpp"
#include "include/user.hpp"
#include "include/sharedlist.hpp"
#include "include/user_routes.hpp"
#include "include/sharedlist_routes.hpp"

using namespace std;

int main(int argc, char **argv) {
	Database db;
	if (!db.open()) {
		cerr << "db failed to open, exiting" << endl;
		return 1;
	}

	Crypto crypto;
	HttpClient http;
	
	User user(db, crypto);
	SessionManager session(db, crypto);
	Sharedlist sharedlist(db, user, http);

	crow::SimpleApp app;

	registerUserRoutes(app, user, session, crypto, http);
	registerSharedlistRoutes(app, sharedlist, user);

	app.port(18808).multithreaded().run();
}
