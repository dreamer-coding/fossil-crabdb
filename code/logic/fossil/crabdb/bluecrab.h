/**
 * -----------------------------------------------------------------------------
 * Project: Fossil Logic
 *
 * This file is part of the Fossil Logic project, which aims to develop
 * high-performance, cross-platform applications and libraries. The code
 * contained herein is licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. You may obtain
 * a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * Author: Michael Gene Brockus (Dreamer)
 * Date: 04/05/2014
 *
 * Copyright (C) 2014-2025 Fossil Logic. All rights reserved.
 * -----------------------------------------------------------------------------
 */
#ifndef FOSSIL_CRABDB_CORE_H
#define FOSSIL_CRABDB_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ======================================================
// Type Definitions
// ======================================================

typedef enum {
    FBC_TYPE_I8,
    FBC_TYPE_I16,
    FBC_TYPE_I32,
    FBC_TYPE_I64,
    FBC_TYPE_U8,
    FBC_TYPE_U16,
    FBC_TYPE_U32,
    FBC_TYPE_U64,
    FBC_TYPE_F32,
    FBC_TYPE_F64,
    FBC_TYPE_CSTR,
    FBC_TYPE_CHAR,
    FBC_TYPE_BOOL,
    FBC_TYPE_HEX,
    FBC_TYPE_OCT,
    FBC_TYPE_BIN,
    FBC_TYPE_SIZE,
    FBC_TYPE_DATETIME,
    FBC_TYPE_DURATION,
    FBC_TYPE_ANY,
    FBC_TYPE_NULL
} fossil_bluecrab_type_t;

// ======================================================
// Core Structures
// ======================================================

typedef struct {
    char *key;
    void *value;
    fossil_bluecrab_type_t type;
    time_t created_at;
    time_t updated_at;
} fossil_bluecrab_entry_t;

typedef struct fossil_bluecrab_table {
    char *name;
    fossil_bluecrab_entry_t **entries;
    size_t entry_count;
    size_t capacity;
} fossil_bluecrab_table_t;

typedef struct fossil_bluecrab_snapshot {
    uint64_t hash;
    time_t timestamp;
    char *message;
} fossil_bluecrab_snapshot_t;

typedef struct {
    fossil_bluecrab_table_t **tables;
    size_t table_count;
    size_t capacity;

    // transaction state
    uint64_t last_hash;
    bool in_transaction;

    // snapshots / commit history
    fossil_bluecrab_snapshot_t **snapshots;
    size_t snapshot_count;
    size_t snapshot_capacity;
} fossil_bluecrab_db_t;

// ======================================================
// Hashing & Tamper Protection
// ======================================================

uint64_t fossil_bluecrab_core_hash(const void *data, size_t len);
bool fossil_bluecrab_core_verify_integrity(fossil_bluecrab_db_t *db);

// ======================================================
// Persistent Storage
// ======================================================

bool fossil_bluecrab_core_save(fossil_bluecrab_db_t *db, const char *filepath);
fossil_bluecrab_db_t* fossil_bluecrab_core_load(const char *filepath);

// ======================================================
// Database Lifecycle
// ======================================================

fossil_bluecrab_db_t* fossil_bluecrab_core_create_db(size_t initial_capacity);
void fossil_bluecrab_core_free_db(fossil_bluecrab_db_t *db);

// ======================================================
// Table Management
// ======================================================

fossil_bluecrab_table_t* fossil_bluecrab_core_create_table(fossil_bluecrab_db_t *db, const char *name);
bool fossil_bluecrab_core_drop_table(fossil_bluecrab_db_t *db, const char *name);

// ======================================================
// CRUD Operations
// ======================================================

bool fossil_bluecrab_core_insert(fossil_bluecrab_table_t *table, const char *key, void *value, fossil_bluecrab_type_t type);
bool fossil_bluecrab_core_update(fossil_bluecrab_table_t *table, const char *key, void *value, fossil_bluecrab_type_t type);
bool fossil_bluecrab_core_delete(fossil_bluecrab_table_t *table, const char *key);
fossil_bluecrab_entry_t* fossil_bluecrab_core_get(fossil_bluecrab_table_t *table, const char *key);

// ======================================================
// Batch Processing & Query
// ======================================================

typedef bool (*fossil_bluecrab_filter_fn)(fossil_bluecrab_entry_t *entry, void *userdata);
size_t fossil_bluecrab_core_query(fossil_bluecrab_table_t *table, fossil_bluecrab_filter_fn filter, void *userdata, fossil_bluecrab_entry_t **results, size_t max_results);
bool fossil_bluecrab_core_batch_process(fossil_bluecrab_table_t *table, fossil_bluecrab_entry_t **entries, size_t count);

// ======================================================
// Transaction Management
// ======================================================

bool fossil_bluecrab_core_begin_transaction(fossil_bluecrab_db_t *db);
bool fossil_bluecrab_core_commit_transaction(fossil_bluecrab_db_t *db);
bool fossil_bluecrab_core_rollback_transaction(fossil_bluecrab_db_t *db);

// ======================================================
// AI / Time / Type Awareness Utilities
// ======================================================

fossil_bluecrab_type_t fossil_bluecrab_core_infer_type(const void *value);
time_t fossil_bluecrab_core_current_time(void);

// ======================================================
// Hybrid SQL + Git-like Snapshot Logic
// ======================================================

bool fossil_bluecrab_core_commit_snapshot(fossil_bluecrab_db_t *db, const char *message);
bool fossil_bluecrab_core_checkout_snapshot(fossil_bluecrab_db_t *db, uint64_t snapshot_hash);

#ifdef __cplusplus
}
#include <string>
#include <vector>
#include <memory>

namespace fossil {
namespace bluecrab {

class Core {
public:
    // Constructor / Destructor
    Core(size_t initial_capacity = 16) {
        db_ = fossil_bluecrab_core_create_db(initial_capacity);
    }

    ~Core() {
        fossil_bluecrab_core_free_db(db_);
    }

    // =========================
    // Table Management
    // =========================
    fossil_bluecrab_table_t* create_table(const std::string& name) {
        return fossil_bluecrab_core_create_table(db_, name.c_str());
    }

    bool drop_table(const std::string& name) {
        return fossil_bluecrab_core_drop_table(db_, name.c_str());
    }

    // =========================
    // CRUD Operations
    // =========================
    bool insert(fossil_bluecrab_table_t* table, const std::string& key, void* value, fossil_bluecrab_type_t type) {
        return fossil_bluecrab_core_insert(table, key.c_str(), value, type);
    }

    bool update(fossil_bluecrab_table_t* table, const std::string& key, void* value, fossil_bluecrab_type_t type) {
        return fossil_bluecrab_core_update(table, key.c_str(), value, type);
    }

    bool remove(fossil_bluecrab_table_t* table, const std::string& key) {
        return fossil_bluecrab_core_delete(table, key.c_str());
    }

    fossil_bluecrab_entry_t* get(fossil_bluecrab_table_t* table, const std::string& key) {
        return fossil_bluecrab_core_get(table, key.c_str());
    }

    // =========================
    // Transaction Management
    // =========================
    bool begin_transaction() {
        return fossil_bluecrab_core_begin_transaction(db_);
    }

    bool commit_transaction() {
        return fossil_bluecrab_core_commit_transaction(db_);
    }

    bool rollback_transaction() {
        return fossil_bluecrab_core_rollback_transaction(db_);
    }

    // =========================
    // Batch Processing & Query
    // =========================
    size_t query(fossil_bluecrab_table_t* table,
                 fossil_bluecrab_filter_fn filter,
                 void* userdata,
                 std::vector<fossil_bluecrab_entry_t*>& results) {
        return fossil_bluecrab_core_query(table, filter, userdata, results.data(), results.size());
    }

    bool batch_process(fossil_bluecrab_table_t* table, std::vector<fossil_bluecrab_entry_t*>& entries) {
        return fossil_bluecrab_core_batch_process(table, entries.data(), entries.size());
    }

    // =========================
    // Persistence
    // =========================
    bool save(const std::string& filepath) {
        return fossil_bluecrab_core_save(db_, filepath.c_str());
    }

    bool load(const std::string& filepath) {
        fossil_bluecrab_db_t* new_db = fossil_bluecrab_core_load(filepath.c_str());
        if (!new_db) return false;
        fossil_bluecrab_core_free_db(db_);
        db_ = new_db;
        return true;
    }

    // =========================
    // AI / Time / Type Awareness
    // =========================
    fossil_bluecrab_type_t infer_type(const void* value) {
        return fossil_bluecrab_core_infer_type(value);
    }

    time_t current_time() {
        return fossil_bluecrab_core_current_time();
    }

    // =========================
    // Snapshot / Hybrid Git-SQL Logic
    // =========================
    bool commit_snapshot(const std::string& message) {
        return fossil_bluecrab_core_commit_snapshot(db_, message.c_str());
    }

    bool checkout_snapshot(uint64_t snapshot_hash) {
        return fossil_bluecrab_core_checkout_snapshot(db_, snapshot_hash);
    }

private:
    fossil_bluecrab_db_t* db_;
};

} // namespace bluecrab

} // namespace fossil

#endif

#endif /* FOSSIL_CRABDB_FRAMEWORK_H */
