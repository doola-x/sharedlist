#pragma once
#include "crow.h"
#include "sharedlist.hpp"
#include "user.hpp"

void registerSharedlistRoutes(crow::SimpleApp& app, Sharedlist& sharedlist, User& user);
