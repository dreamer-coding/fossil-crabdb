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
 * Versioning
 * ========================================================================== */

#define FOSSIL_BLUECRAB_CORE_VERSION_MAJOR 1
#define FOSSIL_BLUECRAB_CORE_VERSION_MINOR 0
#define FOSSIL_BLUECRAB_CORE_VERSION_PATCH 0

/* ============================================================================
 * Result Codes
 * ========================================================================== */

typedef enum fossil_bluecrab_core_result {
    FOSSIL_BLUECRAB_OK = 0,
    FOSSIL_BLUECRAB_ERR_GENERIC,
    FOSSIL_BLUECRAB_ERR_INVALID_ARG,
    FOSSIL_BLUECRAB_ERR_IO,
    FOSSIL_BLUECRAB_ERR_CORRUPT,
    FOSSIL_BLUECRAB_ERR_TAMPERED,
    FOSSIL_BLUECRAB_ERR_NOT_FOUND,
    FOSSIL_BLUECRAB_ERR_TYPE_MISMATCH,
    FOSSIL_BLUECRAB_ERR_UNSUPPORTED
} fossil_bluecrab_core_result_t;

/* ============================================================================
 * Type System
 * ========================================================================== */

typedef enum fossil_bluecrab_core_type {
    /* Signed integers */
    FOSSIL_BLUECRAB_TYPE_I8,
    FOSSIL_BLUECRAB_TYPE_I16,
    FOSSIL_BLUECRAB_TYPE_I32,
    FOSSIL_BLUECRAB_TYPE_I64,

    /* Unsigned integers */
    FOSSIL_BLUECRAB_TYPE_U8,
    FOSSIL_BLUECRAB_TYPE_U16,
    FOSSIL_BLUECRAB_TYPE_U32,
    FOSSIL_BLUECRAB_TYPE_U64,

    /* Floating point */
    FOSSIL_BLUECRAB_TYPE_F32,
    FOSSIL_BLUECRAB_TYPE_F64,

    /* Text */
    FOSSIL_BLUECRAB_TYPE_CSTR,
    FOSSIL_BLUECRAB_TYPE_CHAR,

    /* Boolean */
    FOSSIL_BLUECRAB_TYPE_BOOL,

    /* Extended */
    FOSSIL_BLUECRAB_TYPE_HEX,
    FOSSIL_BLUECRAB_TYPE_OCT,
    FOSSIL_BLUECRAB_TYPE_BIN,
    FOSSIL_BLUECRAB_TYPE_SIZE,
    FOSSIL_BLUECRAB_TYPE_DATETIME,
    FOSSIL_BLUECRAB_TYPE_DURATION,

    /* Generic */
    FOSSIL_BLUECRAB_TYPE_ANY,
    FOSSIL_BLUECRAB_TYPE_NULL
} fossil_bluecrab_core_type_t;

/* ============================================================================
 * Time Awareness
 * ========================================================================== */

typedef struct fossil_bluecrab_core_time {
    int64_t wall_time_ns;      /* Unix epoch nanoseconds */
    int64_t monotonic_ns;      /* Monotonic time for ordering */
} fossil_bluecrab_core_time_t;

/* ============================================================================
 * Value Container
 * ========================================================================== */

typedef struct fossil_bluecrab_core_value {
    fossil_bluecrab_core_type_t type;
    size_t size;
    const void *data;
} fossil_bluecrab_core_value_t;

/* ============================================================================
 * Record & Provenance
 * ========================================================================== */

#define FOSSIL_BLUECRAB_HASH_SIZE 32  /* algorithm-agnostic */

typedef struct fossil_bluecrab_core_hash {
    uint8_t bytes[FOSSIL_BLUECRAB_HASH_SIZE];
} fossil_bluecrab_core_hash_t;

typedef struct fossil_bluecrab_core_record {
    uint64_t record_id;
    fossil_bluecrab_core_time_t timestamp;
    fossil_bluecrab_core_value_t value;

    /* Tamper-evident chain */
    fossil_bluecrab_core_hash_t prev_hash;
    fossil_bluecrab_core_hash_t self_hash;

    /* AI / provenance hooks */
    float confidence_score;
    uint32_t usage_count;
} fossil_bluecrab_core_record_t;

/* ============================================================================
 * Git-Inspired Database State
 * ========================================================================== */

typedef struct fossil_bluecrab_core_commit {
    fossil_bluecrab_core_hash_t commit_hash;
    fossil_bluecrab_core_hash_t parent_hash;
    fossil_bluecrab_core_time_t timestamp;
    uint64_t record_count;
} fossil_bluecrab_core_commit_t;

/* Opaque database handle */
typedef struct fossil_bluecrab_core_db fossil_bluecrab_core_db_t;

/* ============================================================================
 * Storage Interface (Pluggable)
 * ========================================================================== */

typedef struct fossil_bluecrab_core_storage_ops {
    void *ctx;

    fossil_bluecrab_core_result_t (*read)(
        void *ctx, uint64_t offset, void *buf, size_t size);

    fossil_bluecrab_core_result_t (*write)(
        void *ctx, uint64_t offset, const void *buf, size_t size);

    fossil_bluecrab_core_result_t (*flush)(void *ctx);
} fossil_bluecrab_core_storage_ops_t;

/* ============================================================================
 * Core Lifecycle
 * ========================================================================== */

fossil_bluecrab_core_result_t
fossil_bluecrab_core_init(
    fossil_bluecrab_core_db_t **out_db,
    const fossil_bluecrab_core_storage_ops_t *storage_ops);

void
fossil_bluecrab_core_shutdown(
    fossil_bluecrab_core_db_t *db);

/* ============================================================================
 * Record Operations
 * ========================================================================== */

fossil_bluecrab_core_result_t
fossil_bluecrab_core_insert(
    fossil_bluecrab_core_db_t *db,
    const fossil_bluecrab_core_value_t *value,
    uint64_t *out_record_id);

fossil_bluecrab_core_result_t
fossil_bluecrab_core_fetch(
    fossil_bluecrab_core_db_t *db,
    uint64_t record_id,
    fossil_bluecrab_core_record_t *out_record);

/* ============================================================================
 * Tamper & Integrity
 * ========================================================================== */

fossil_bluecrab_core_result_t
fossil_bluecrab_core_verify_chain(
    fossil_bluecrab_core_db_t *db);

fossil_bluecrab_core_result_t
fossil_bluecrab_core_rehash_all(
    fossil_bluecrab_core_db_t *db);

/* ============================================================================
 * Git-Style Operations
 * ========================================================================== */

fossil_bluecrab_core_result_t
fossil_bluecrab_core_commit(
    fossil_bluecrab_core_db_t *db,
    fossil_bluecrab_core_commit_t *out_commit);

fossil_bluecrab_core_result_t
fossil_bluecrab_core_checkout(
    fossil_bluecrab_core_db_t *db,
    const fossil_bluecrab_core_hash_t *commit_hash);

fossil_bluecrab_core_result_t
fossil_bluecrab_core_diff(
    fossil_bluecrab_core_db_t *db,
    const fossil_bluecrab_core_hash_t *a,
    const fossil_bluecrab_core_hash_t *b);

/* ============================================================================
 * AI-Oriented Introspection
 * ========================================================================== */

fossil_bluecrab_core_result_t
fossil_bluecrab_core_score_record(
    fossil_bluecrab_core_db_t *db,
    uint64_t record_id,
    float delta);

fossil_bluecrab_core_result_t
fossil_bluecrab_core_touch_record(
    fossil_bluecrab_core_db_t *db,
    uint64_t record_id);

/* ============================================================================
 * Utilities
 * ========================================================================== */

const char *
fossil_bluecrab_core_type_name(
    fossil_bluecrab_core_type_t type);

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
