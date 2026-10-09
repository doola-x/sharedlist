#pragma once
#include "crypto.hpp"
#include "http_client.hpp"
#include "session.hpp"
#include "user.hpp"
#include "connection_pool.hpp"
#include "sharedlist.hpp"
#include <memory>

using namespace std;

struct ScopedRequest {
    std::shared_ptr<ConnectionPool> connPool_;
    std::shared_ptr<Crypto> crypto_;
    std::shared_ptr<HttpClient> http_;

    explicit ScopedRequest() {
        connPool_ = make_shared<ConnectionPool>();
        crypto_ = make_shared<Crypto>();
        http_ = make_shared<HttpClient>();
    }

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


