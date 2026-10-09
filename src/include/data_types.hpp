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
    std::shared_ptr<ConnectionPool> connPool;
    std::shared_ptr<Crypto> crypto;
    std::shared_ptr<HttpClient> http;

    explicit ScopedRequest() {
        connPool = make_shared<ConnectionPool>();
        crypto = make_shared<Crypto>();
        http = make_shared<HttpClient>();
    }

    struct context {
        std::unique_ptr<User> user_;
        std::unique_ptr<SessionManager> session_;
        std::unique_ptr<Sharedlist> sharedlist_;
    };

    void before_handle(crow::request&, crow::response&, context& c, auto&) {
        c.user_ = std::make_unique<User>(connPool->acquire(), crypto.get());
        c.session_ = std::make_unique<SessionManager>(connPool->acquire(), crypto.get());
        c.sharedlist_ = std::make_unique<Sharedlist>(connPool->acquire(), http.get());
    }

    void after_handle(crow::request&, crow::response&, context&, auto&) {

    }
};


