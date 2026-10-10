#pragma once
#include <atomic>
#include <memory>
#include <optional>
#include <utility>
#include "dal.hpp"

struct Connection {
    Database* db;
    int id;
};

const static int MAX_CONNECTIONS = 128;

using PossibleConnection = std::optional<std::shared_ptr<Connection>>;

class ConnectionPool {
public:
    ConnectionPool() {
        int running_id = 0;
        for (auto& c: _connections) {
            c = std::make_unique<Connection>();
            c->id = running_id++;
        }
    }
  
    PossibleConnection acquire() {
        std::cout << "attempting acquire..." << std::endl;
        int head = _head.load(std::memory_order_relaxed);
        int tail = _tail.load(std::memory_order_acquire);

        if (tail == head) return std::nullopt;

        PossibleConnection pc = std::move(_connections[tail]);
        _tail.fetch_add(1, std::memory_order_acquire);
        std::cout << "conn acq'd" << std::endl;
        return pc; 
    }

    bool release(std::unique_ptr<Connection> c) {
        int head = _head.load(std::memory_order_acquire);
        int tail = _tail.load(std::memory_order_relaxed);

        if ((tail - head) % MAX_CONNECTIONS == 0) return false;
    
        _connections[head] = std::move(c);    
        _head.fetch_add(1, std::memory_order_acquire);
        return true;
    }
private:
    std::unique_ptr<Connection> _connections[MAX_CONNECTIONS]{};
    alignas(64) std::atomic<int> _head = 0;
    alignas(64) std::atomic<int> _tail = 1;
};
