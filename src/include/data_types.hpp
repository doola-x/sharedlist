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
        int user_id = -1; // authenticated user from the session cookie, -1 if none
        string session_token;
    };

    void before_handle(crow::request& req, crow::response&, context& c, auto&) {
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

        c.session_token = cookieValue(req.get_header_value("Cookie"), "session_token");
        c.user_id = c.session_->userIdFromToken(c.session_token);
    }

    static string cookieValue(const string& header, const string& name) {
        size_t pos = 0;
        while (pos < header.size()) {
            size_t end = header.find(';', pos);
            if (end == string::npos) end = header.size();
            string pair = header.substr(pos, end - pos);
            size_t start = pair.find_first_not_of(' ');
            if (start != string::npos) pair = pair.substr(start);
            if (pair.compare(0, name.size() + 1, name + "=") == 0) return pair.substr(name.size() + 1);
            pos = end + 1;
        }
        return "";
    }

    void after_handle(crow::request&, crow::response&, context& c, auto&) {
        c.sharedlist_.reset();
        c.session_.reset();
        c.user_.reset();
        if (c.conn.has_value()) {
            connPool->release(std::move(c.conn.value()));
        }
        c.conn.reset();
    }
};


