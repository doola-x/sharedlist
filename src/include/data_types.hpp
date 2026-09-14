#pragma once
#include "crypto.hpp"
#include "http_client.hpp"
#include "session.hpp"
#include "user.hpp"
#include "connection_pool.hpp"
#include "sharedlist.hpp"

using namespace std;

struct ScopedRequest {
    const static ConnectionPool* _connPool;

    struct context {
        Crypto* _crypto;
        HttpClient* _http;
        User* _user;
        SessionManager* _session;
        Sharedlist* _sharedlist;
        const Database* _database = _connPool->acquire();
    };
};


