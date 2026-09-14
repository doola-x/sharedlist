#include "include/user_routes.hpp"
#include "include/sharedlist_routes.hpp"

using namespace std;


int main(int argc, char **argv) {
	crow::App<ScopedRequest> app;

	registerUserRoutes(app);
	registerSharedlistRoutes(app);

	app.port(18808).multithreaded().run();
}
