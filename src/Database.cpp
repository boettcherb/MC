#include "Database.h"
#include <sqlite3/sqlite3.h>
#include <thread>
#include <mutex>
#include <queue>
#include <iostream>
#include <cassert>
#include <cstring>

namespace database {

    static const char* DATABASE_FILE_NAME = "MCDB.db";
    static const char* CREATE_TABLE = "CREATE TABLE IF NOT EXISTS mcdb_table (x INT,"
        "z INT, data BLOB NOT NULL, CONSTRAINT mcdb_pk PRIMARY KEY (x, z));";
    static const char* SELECT_ROW = "SELECT data FROM mcdb_table WHERE x = ? AND z = ?";
    static const char* INSERT_ROW = "INSERT OR REPLACE INTO mcdb_table VALUES (?, ?, ?)";

    static std::queue<Query> request_queue;
    static std::mutex request_queue_mutex;

    static std::queue<Query> result_queue;
    static std::mutex result_queue_mutex;

    static bool thread_should_close;

    static void check(int error_code, int sqlite_call_index) {
        if (error_code != SQLITE_OK && error_code != SQLITE_DONE) {
            std::cout << "Error on sqlite call #" << sqlite_call_index << std::endl;
            std::cout << "SQLiteError: " << sqlite3_errstr(error_code) << std::endl;
        }
    }

    static void db_thread_func() {
        check(sqlite3_initialize(), 1);
        sqlite3* db = nullptr;
        check(sqlite3_open(DATABASE_FILE_NAME, &db), 2);
        check(sqlite3_exec(db, CREATE_TABLE, nullptr, nullptr, nullptr), 3);
        sqlite3_stmt* select_stmt = nullptr;
        sqlite3_stmt* insert_stmt = nullptr;
        check(sqlite3_prepare_v2(db, SELECT_ROW, -1, &select_stmt, nullptr), 4);
        check(sqlite3_prepare_v2(db, INSERT_ROW, -1, &insert_stmt, nullptr), 5);

        while (!thread_should_close || request_queue.size() > 0) {
            Query request = { QUERY_NONE, 0, 0, 0, nullptr };
            request_queue_mutex.lock();
            if (!request_queue.empty()) {
                request = request_queue.front();
                request_queue.pop();
            }
            request_queue_mutex.unlock();

            if (request.type == QUERY_LOAD) {
                assert(request.data == nullptr);
                check(sqlite3_bind_int(select_stmt, 1, request.x), 6);
                check(sqlite3_bind_int(select_stmt, 2, request.z), 7);
                if (sqlite3_step(select_stmt) == SQLITE_DONE) {
                    result_queue_mutex.lock();
                    result_queue.emplace(QUERY_LOAD, request.x, request.z, 0, nullptr);
                    result_queue_mutex.unlock();
                } else {
                    int blob_size = sqlite3_column_bytes(select_stmt, 0);
                    const void* blob_data = sqlite3_column_blob(select_stmt, 0);
                    void* block_data = new unsigned char[blob_size];
                    memcpy(block_data, blob_data, blob_size);
                    result_queue_mutex.lock();
                    result_queue.emplace(QUERY_LOAD, request.x, request.z, 0, block_data);
                    result_queue_mutex.unlock();
                }
                check(sqlite3_reset(select_stmt), 8);
            }
            else if (request.type == QUERY_STORE) {
                assert(request.data != nullptr);
                check(sqlite3_bind_int(insert_stmt, 1, request.x), 9);
                check(sqlite3_bind_int(insert_stmt, 2, request.z), 10);
                check(sqlite3_bind_blob(insert_stmt, 3, request.data, request.size, SQLITE_STATIC), 11);
                check(sqlite3_step(insert_stmt), 12);
                check(sqlite3_reset(insert_stmt), 13);
                delete[] request.data;
            }
        }
        check(sqlite3_finalize(select_stmt), 14);
        check(sqlite3_finalize(insert_stmt), 15);
        check(sqlite3_close(db), 17);
    }

    void request_load(int x, int z) {
        request_queue_mutex.lock();
        request_queue.emplace(QUERY_LOAD, x, z, 0, nullptr);
        request_queue_mutex.unlock();
    }
    
    void request_store(int x, int z, int size, const void* data) {
        request_queue_mutex.lock();
        request_queue.emplace(QUERY_STORE, x, z, size, data);
        request_queue_mutex.unlock();
    }

    Query get_load_result() {
        Query result = { QUERY_NONE, 0, 0, 0, nullptr };
        result_queue_mutex.lock();
        if (!result_queue.empty()) {
            result = result_queue.front();
            result_queue.pop();
        }
        result_queue_mutex.unlock();
        return result;
    }

    static std::thread db_thread;

    void initialize() {
        thread_should_close = false;
        db_thread = std::thread(db_thread_func);
    }

    void close() {
        thread_should_close = true;
        db_thread.join();
    }
}
