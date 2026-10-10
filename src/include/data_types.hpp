#pragma once
#include "user.hpp"
#include "crypto.hpp"
#include "session.hpp"
#include "sharedlist.hpp"
#include "http_client.hpp"
#include "connection_pool.hpp"
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
        PossibleConnection conn;
        std::unique_ptr<User> user_;
        std::unique_ptr<SessionManager> session_;
        std::unique_ptr<Sharedlist> sharedlist_;
    };

    void before_handle(crow::request&, crow::response&, context& c, auto&) {
        Database* db = nullptr;
        while (!db) {
            c.conn = connPool->acquire();
            if (c.conn == std::nullopt) {
                continue;
            }
            db = c.conn.value()->db;
        }
        c.user_ = std::make_unique<User>(db, crypto.get());
        c.session_ = std::make_unique<SessionManager>(db, crypto.get());
        c.sharedlist_ = std::make_unique<Sharedlist>(db, http.get());
    }

    void after_handle(crow::request&, crow::response&, context& c, auto&) {
        c.sharedlist_.reset();
        c.session_.reset();
        c.user_.reset();
        connPool->release(std::move(c.conn.value()));
        c.conn.reset();
    }
};


