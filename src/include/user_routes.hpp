#pragma once
#include "crow.h"
#include "user.hpp"
#include "session.hpp"
#include "crypto.hpp"
#include "http_client.hpp"

void registerUserRoutes(crow::SimpleApp& app, User& user, SessionManager& session, Crypto& crypto, HttpClient& http);
