#pragma once
#include "crypto.hpp"
#include "http_client.hpp"
#include "session.hpp"
#include "user.hpp"
#include "connection_pool.hpp"
#include "sharedlist.hpp"

using namespace std;

struct ScopedRequest {
    ConnectionPool* _connPool;
    Crypto crypto;
    HttpClient http;

    struct context {
        User* user_;
        SessionManager* session_;
        Sharedlist* sharedlist_;
    };

    void before_handle(crow::request&, crow::response&, context& c, auto&) {
        
    }

    void after_handle(crow::request&, crow::response&, context&, auto&) {

    }
};


