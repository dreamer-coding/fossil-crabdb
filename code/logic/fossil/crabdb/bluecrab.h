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
#include <memory>
#include <stdexcept>

namespace fossil {

namespace bluecrab {

class Core {
public:
    // Constructor: initialize with storage ops
    Core(const fossil_bluecrab_core_storage_ops_t& storage_ops) {
        fossil_bluecrab_core_db_t* db_ptr = nullptr;
        auto res = fossil_bluecrab_core_init(&db_ptr, &storage_ops);
        if (res != FOSSIL_BLUECRAB_OK) {
            throw std::runtime_error("Failed to initialize Blue Crab core");
        }
        db_.reset(db_ptr, [](fossil_bluecrab_core_db_t* db) {
            fossil_bluecrab_core_shutdown(db);
        });
    }

    // Insert a value, returns record ID
    uint64_t insert(const fossil_bluecrab_core_value_t& value) {
        uint64_t record_id = 0;
        auto res = fossil_bluecrab_core_insert(db_.get(), &value, &record_id);
        if (res != FOSSIL_BLUECRAB_OK) {
            throw std::runtime_error("Failed to insert record");
        }
        return record_id;
    }

    // Fetch a record by ID
    fossil_bluecrab_core_record_t fetch(uint64_t record_id) {
        fossil_bluecrab_core_record_t record{};
        auto res = fossil_bluecrab_core_fetch(db_.get(), record_id, &record);
        if (res != FOSSIL_BLUECRAB_OK) {
            throw std::runtime_error("Failed to fetch record");
        }
        return record;
    }

    // Verify tamper-evident chain
    void verify_chain() {
        auto res = fossil_bluecrab_core_verify_chain(db_.get());
        if (res != FOSSIL_BLUECRAB_OK) {
            throw std::runtime_error("Tamper detected in chain");
        }
    }

    // Rehash all records
    void rehash_all() {
        auto res = fossil_bluecrab_core_rehash_all(db_.get());
        if (res != FOSSIL_BLUECRAB_OK) {
            throw std::runtime_error("Failed to rehash records");
        }
    }

    // Commit current state (git-style)
    fossil_bluecrab_core_commit_t commit() {
        fossil_bluecrab_core_commit_t commit{};
        auto res = fossil_bluecrab_core_commit(db_.get(), &commit);
        if (res != FOSSIL_BLUECRAB_OK) {
            throw std::runtime_error("Failed to commit database");
        }
        return commit;
    }

    // Checkout a commit
    void checkout(const fossil_bluecrab_core_hash_t& commit_hash) {
        auto res = fossil_bluecrab_core_checkout(db_.get(), &commit_hash);
        if (res != FOSSIL_BLUECRAB_OK) {
            throw std::runtime_error("Failed to checkout commit");
        }
    }

    // Diff between two commits
    void diff(const fossil_bluecrab_core_hash_t& a, const fossil_bluecrab_core_hash_t& b) {
        auto res = fossil_bluecrab_core_diff(db_.get(), &a, &b);
        if (res != FOSSIL_BLUECRAB_OK) {
            throw std::runtime_error("Failed to diff commits");
        }
    }

    // AI-oriented scoring
    void score_record(uint64_t record_id, float delta) {
        auto res = fossil_bluecrab_core_score_record(db_.get(), record_id, delta);
        if (res != FOSSIL_BLUECRAB_OK) {
            throw std::runtime_error("Failed to score record");
        }
    }

    void touch_record(uint64_t record_id) {
        auto res = fossil_bluecrab_core_touch_record(db_.get(), record_id);
        if (res != FOSSIL_BLUECRAB_OK) {
            throw std::runtime_error("Failed to touch record");
        }
    }

    // Utility: get type name
    static const char* type_name(fossil_bluecrab_core_type_t type) {
        return fossil_bluecrab_core_type_name(type);
    }

private:
    std::unique_ptr<fossil_bluecrab_core_db_t, void(*)(fossil_bluecrab_core_db_t*)> db_{nullptr, [](fossil_bluecrab_core_db_t* db){ /* dummy */ }};
};

} // namespace bluecrab

} // namespace fossil

#endif

#endif /* FOSSIL_CRABDB_FRAMEWORK_H */
