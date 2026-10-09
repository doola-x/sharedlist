#pragma once
#include "sqlite3.h"
#include <string>
#include <vector>
#include <variant>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <iostream>
#include <type_traits>
using namespace std;

// A single bound SQL parameter. Remembers the type it was constructed from so
// binding can dispatch to the matching sqlite3_bind_* call instead of pushing
// everything through bind_text.
class DbValue {
public:
	DbValue(nullptr_t)     : v(nullptr) {}
	DbValue(int i)         : v(static_cast<int64_t>(i)) {}
	DbValue(int64_t i)     : v(i) {}
	DbValue(double d)      : v(d) {}
	DbValue(string s)      : v(std::move(s)) {}
	DbValue(const char* s) : v(string(s)) {}

	void bind(sqlite3_stmt* stmt, int idx) const {
		visit([&](auto&& val) {
			using T = decay_t<decltype(val)>;
			if constexpr (is_same_v<T, nullptr_t>)    sqlite3_bind_null(stmt, idx);
			else if constexpr (is_same_v<T, int64_t>) sqlite3_bind_int64(stmt, idx, val);
			else if constexpr (is_same_v<T, double>)  sqlite3_bind_double(stmt, idx, val);
			// TRANSIENT: sqlite copies the text, so it stays valid even if the
			// caller's params vector was a temporary.
			else sqlite3_bind_text(stmt, idx, val.c_str(), -1, SQLITE_TRANSIENT);
		}, v);
	}

private:
	variant<nullptr_t, int64_t, double, string> v;
};
using DbParams = vector<DbValue>;

inline void bindParams(sqlite3_stmt* stmt, const DbParams& params) {
	for (size_t i = 0; i < params.size(); i++) {
		params[i].bind(stmt, static_cast<int>(i + 1));
	}
}

class Database { 
public:
	Database(const string& databaseName = "/data/sharedlist/database/sharedlist.db");
	~Database();

	bool open();
	bool execute(const string& sql) const;
	int prepareStatement(const string& sql, const DbParams& params = {}) const;

	template<typename T>
	vector<T> query(const string& sql, const DbParams& params = {}) const {
		sqlite3_stmt* stmt = nullptr;
		vector<T> items;

		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
			cerr << "Failed to prepare statement: " << sqlite3_errmsg(db);
			return items;
		}
		bindParams(stmt, params);
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			items.push_back(T::fromRow(stmt));
		}

		sqlite3_finalize(stmt);
		return items;
	}
	void close();
    

private:
	sqlite3 *db;
	string database_name;
	bool is_open;
};
