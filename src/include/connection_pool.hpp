#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include "dal.hpp"

struct Connection {
    Database* db;
    int id;

    Connection() {
        db = new Database();
        db->open();
        id = -1;
    }
};

const static size_t MAX_CONNECTIONS = 16; // power of two, slots are addressed with a mask

using PossibleConnection = std::optional<std::unique_ptr<Connection>>;

// bounded MPMC ring (per-slot sequence numbers). a slot's seq says whose turn it
// is: seq == pos means free for the release at pos, seq == pos + 1 means full for
// the acquire at pos. threads claim a position with a CAS, then touch only that slot.
class ConnectionPool {
public:
    ConnectionPool() {
        for (size_t i = 0; i < MAX_CONNECTIONS; i++) {
            _cells[i].seq.store(i, std::memory_order_relaxed);
        }
        for (size_t i = 0; i < MAX_CONNECTIONS; i++) {
            auto c = std::make_unique<Connection>();
            c->id = static_cast<int>(i);
            release(std::move(c));
        }
    }

    // nullopt when every connection is checked out
    PossibleConnection acquire() {
        size_t pos = _tail.load(std::memory_order_relaxed);
        Cell* cell;
        for (;;) {
            cell = &_cells[pos & (MAX_CONNECTIONS - 1)];
            size_t seq = cell->seq.load(std::memory_order_acquire);
            intptr_t dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
            if (dif == 0) {
                if (_tail.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) break;
            } else if (dif < 0) {
                return std::nullopt;
            } else {
                pos = _tail.load(std::memory_order_relaxed);
            }
        }
        PossibleConnection pc = std::move(cell->conn);
        cell->seq.store(pos + MAX_CONNECTIONS, std::memory_order_release);
        return pc;
    }

    // false when the pool is already full
    bool release(std::unique_ptr<Connection> c) {
        size_t pos = _head.load(std::memory_order_relaxed);
        Cell* cell;
        for (;;) {
            cell = &_cells[pos & (MAX_CONNECTIONS - 1)];
            size_t seq = cell->seq.load(std::memory_order_acquire);
            intptr_t dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            if (dif == 0) {
                if (_head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) break;
            } else if (dif < 0) {
                return false;
            } else {
                pos = _head.load(std::memory_order_relaxed);
            }
        }
        cell->conn = std::move(c);
        cell->seq.store(pos + 1, std::memory_order_release);
        return true;
    }
private:
    struct Cell {
        std::atomic<size_t> seq{0};
        std::unique_ptr<Connection> conn;
    };
    Cell _cells[MAX_CONNECTIONS];
    alignas(64) std::atomic<size_t> _head{0}; // next release position
    alignas(64) std::atomic<size_t> _tail{0}; // next acquire position
};
