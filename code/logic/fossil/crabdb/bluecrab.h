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

/* ============================================================================
 * BLUECRAB CORE TYPES
 * ============================================================================ */
typedef enum {
    FBC_TYPE_I8, FBC_TYPE_I16, FBC_TYPE_I32, FBC_TYPE_I64,
    FBC_TYPE_U8, FBC_TYPE_U16, FBC_TYPE_U32, FBC_TYPE_U64,
    FBC_TYPE_F32, FBC_TYPE_F64,
    FBC_TYPE_CSTR, FBC_TYPE_CHAR,
    FBC_TYPE_BOOL,
    FBC_TYPE_HEX, FBC_TYPE_OCT, FBC_TYPE_BIN,
    FBC_TYPE_SIZE, FBC_TYPE_DATETIME, FBC_TYPE_DURATION,
    FBC_TYPE_ANY, FBC_TYPE_NULL
} fossil_bluecrab_core_type_t;

/* ============================================================================
 * VALUE STORAGE
 * ============================================================================ */
typedef struct {
    fossil_bluecrab_core_type_t type;
    union {
        int8_t   i8;
        int16_t  i16;
        int32_t  i32;
        int64_t  i64;
        uint8_t  u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
        float    f32;
        double   f64;
        char    *cstr;
        char     chr;
        bool     boolean;
        uint64_t size;
        struct tm datetime;
        double   duration;
        void    *any;
    } value;
} fossil_bluecrab_core_value_t;

/* ============================================================================
 * COLUMN AND TABLE
 * ============================================================================ */
#define FBC_MAX_COLUMNS 64

typedef struct {
    char name[64];
    fossil_bluecrab_core_type_t type;
    bool is_primary_key;
    bool is_indexed;
} fossil_bluecrab_core_column_t;

typedef struct {
    char name[64];
    size_t row_count;
    size_t column_count;
    fossil_bluecrab_core_column_t columns[FBC_MAX_COLUMNS];
    fossil_bluecrab_core_value_t **rows; // dynamic array of row pointers
} fossil_bluecrab_core_table_t;

/* ============================================================================
 * DATABASE STRUCTURE
 * ============================================================================ */
#define FBC_MAX_TABLES 64

typedef struct {
    char name[64];
    size_t table_count;
    fossil_bluecrab_core_table_t tables[FBC_MAX_TABLES];
    uint64_t hash_salt;    // tamper protection
    time_t last_modified;  // time awareness
} fossil_bluecrab_core_db_t;

/* ============================================================================
 * ESSENTIAL CORE FUNCTIONS
 * ============================================================================ */

// Database lifecycle
fossil_bluecrab_core_db_t* fossil_bluecrab_core_create_db(const char *name);
bool fossil_bluecrab_core_destroy_db(fossil_bluecrab_core_db_t *db);

// Table management
fossil_bluecrab_core_table_t* fossil_bluecrab_core_create_table(fossil_bluecrab_core_db_t *db, const char *table_name);
bool fossil_bluecrab_core_drop_table(fossil_bluecrab_core_db_t *db, const char *table_name);

// Row operations
bool fossil_bluecrab_core_insert_row(fossil_bluecrab_core_table_t *table, fossil_bluecrab_core_value_t *values);
bool fossil_bluecrab_core_delete_row(fossil_bluecrab_core_table_t *table, size_t row_index);
fossil_bluecrab_core_value_t* fossil_bluecrab_core_get_value(fossil_bluecrab_core_table_t *table, size_t row_index, const char *column_name);

// Hash / tamper protection
uint64_t fossil_bluecrab_core_hash(const void *data, size_t len, uint64_t salt);
bool fossil_bluecrab_core_verify_hash(fossil_bluecrab_core_db_t *db);

// AI / intelligence hooks
void fossil_bluecrab_core_ai_optimize(fossil_bluecrab_core_db_t *db);

#ifdef __cplusplus
}
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace fossil {

namespace bluecrab {

class Core {
public:
    Core(const std::string &db_name) {
        db_ = fossil_bluecrab_core_create_db(db_name.c_str());
        if (!db_) throw std::runtime_error("Failed to create BlueCrab DB");
    }

    ~Core() {
        if (db_) fossil_bluecrab_core_destroy_db(db_);
    }

    // ---------------- Database ----------------
    const std::string& name() const { return db_name_; }

    bool verify_hash() const {
        return fossil_bluecrab_core_verify_hash(db_);
    }

    // ---------------- Tables ----------------
    fossil_bluecrab_core_table_t* create_table(const std::string &table_name) {
        auto table = fossil_bluecrab_core_create_table(db_, table_name.c_str());
        if (!table) throw std::runtime_error("Failed to create table");
        return table;
    }

    bool drop_table(const std::string &table_name) {
        return fossil_bluecrab_core_drop_table(db_, table_name.c_str());
    }

    size_t table_count() const { return db_ ? db_->table_count : 0; }

    fossil_bluecrab_core_table_t* get_table(size_t index) {
        if (!db_ || index >= db_->table_count) return nullptr;
        return &db_->tables[index];
    }

    // ---------------- Rows ----------------
    bool insert_row(fossil_bluecrab_core_table_t* table, fossil_bluecrab_core_value_t* values) {
        return fossil_bluecrab_core_insert_row(table, values);
    }

    bool delete_row(fossil_bluecrab_core_table_t* table, size_t row_index) {
        return fossil_bluecrab_core_delete_row(table, row_index);
    }

    fossil_bluecrab_core_value_t* get_value(fossil_bluecrab_core_table_t* table, size_t row_index, const std::string &column_name) {
        return fossil_bluecrab_core_get_value(table, row_index, column_name.c_str());
    }

    // ---------------- Hash / AI ----------------
    uint64_t compute_hash(const void* data, size_t len, uint64_t salt) const {
        return fossil_bluecrab_core_hash(data, len, salt);
    }

    void ai_optimize() {
        fossil_bluecrab_core_ai_optimize(db_);
    }

private:
    fossil_bluecrab_core_db_t* db_ = nullptr;
    std::string db_name_;
};

} // namespace bluecrab

} // namespace fossil

#endif

#endif /* FOSSIL_CRABDB_FRAMEWORK_H */
