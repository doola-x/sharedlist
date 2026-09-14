#pragma once
#include <atomic>
#include "dal.hpp"

struct Connection {
    int id;
    Database* db;
};

const static int MAX_CONNECTIONS = 100;

class ConnectionPool {
public:
    ConnectionPool() {}
  
    const Database* acquire() const;
private:
    Connection* _connections[MAX_CONNECTIONS];
    alignas(64) std::atomic<int> _head = 0;
    alignas(64) std::atomic<int> _tail = 0;
};
