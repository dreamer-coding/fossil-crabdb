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
 * Blue Crab Database Core
 * ----------------------------------------------------------------------------
 * Portable, multi-purpose database core for sub-libraries. Supports:
 *  - Persistent storage
 *  - Cold-core tamper protection
 *  - Time awareness
 *  - Type awareness
 *  - Git-based logic for database operations
 *  - Hybrid of Git and NoSQL
 * ============================================================================ */

/* ============================================================================
 * Data Types
 * ---------------------------------------------------------------------------
 * Signed Int: "i8", "i16", "i32", "i64"
 * Unsigned Int: "u8", "u16", "u32", "u64"
 * Floating: "f32", "f64"
 * String/Text: "cstr", "char"
 * Boolean: "bool"
 * Extended: "hex", "oct", "bin", "size", "datetime", "duration"
 * Generic: "any", "null"
 * ============================================================================ */
typedef enum {
    FBC_TYPE_NULL = 0,
    FBC_TYPE_ANY,

    /* Signed integers */
    FBC_TYPE_I8,
    FBC_TYPE_I16,
    FBC_TYPE_I32,
    FBC_TYPE_I64,

    /* Unsigned integers */
    FBC_TYPE_U8,
    FBC_TYPE_U16,
    FBC_TYPE_U32,
    FBC_TYPE_U64,

    /* Floating points */
    FBC_TYPE_F32,
    FBC_TYPE_F64,

    /* Strings */
    FBC_TYPE_CSTR,
    FBC_TYPE_CHAR,

    /* Boolean */
    FBC_TYPE_BOOL,

    /* Extended types */
    FBC_TYPE_HEX,
    FBC_TYPE_OCT,
    FBC_TYPE_BIN,
    FBC_TYPE_SIZE,
    FBC_TYPE_DATETIME,
    FBC_TYPE_DURATION
} fossil_bluecrab_core_type_t;

/* ============================================================================
 * Generic Value Container
 * ---------------------------------------------------------------------------
 * Holds any value with type awareness
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

        char     *cstr;
        char     ch;

        bool     boolean;

        /* Extended values as string representation */
        char     *hex;
        char     *oct;
        char     *bin;
        size_t   size;
        time_t   datetime;
        double   duration;
    } value;
} fossil_bluecrab_core_value_t;

typedef struct {
    char *hash;       // Commit hash (simple placeholder or SHA256)
    char *message;    // Commit message
    time_t timestamp; // Commit time
    fossil_bluecrab_core_entry_t *snapshot; // Deep copy of entries
    size_t snapshot_count;
} fossil_bluecrab_core_commit_t;

typedef struct {
    char *db_path;
    size_t entry_count;
    fossil_bluecrab_core_entry_t *entries;

    /* Git-like versioning */
    char *current_commit;
    char *branch;

    /* Commit history */
    fossil_bluecrab_core_commit_t *commits;
    size_t commit_count;
} fossil_bluecrab_core_db_t;

/* ============================================================================
 * Initialization / Shutdown
 * ============================================================================ */
bool fossil_bluecrab_core_init(fossil_bluecrab_core_db_t *db, const char *path);
bool fossil_bluecrab_core_shutdown(fossil_bluecrab_core_db_t *db);

/* ============================================================================
 * CRUD Operations
 * ============================================================================ */
bool fossil_bluecrab_core_set(fossil_bluecrab_core_db_t *db,
                              const char *key,
                              fossil_bluecrab_core_value_t value);

bool fossil_bluecrab_core_get(fossil_bluecrab_core_db_t *db,
                              const char *key,
                              fossil_bluecrab_core_value_t *out_value);

bool fossil_bluecrab_core_delete(fossil_bluecrab_core_db_t *db, const char *key);

/* ============================================================================
 * Git-like Operations
 * ============================================================================ */
bool fossil_bluecrab_core_commit(fossil_bluecrab_core_db_t *db, const char *message);
bool fossil_bluecrab_core_checkout(fossil_bluecrab_core_db_t *db, const char *commit_hash);
bool fossil_bluecrab_core_branch(fossil_bluecrab_core_db_t *db, const char *branch_name);
bool fossil_bluecrab_core_log(fossil_bluecrab_core_db_t *db);

/* ============================================================================
 * Persistent Recovery
 * ============================================================================ */
bool fossil_bluecrab_core_save(fossil_bluecrab_core_db_t *db);
bool fossil_bluecrab_core_load(fossil_bluecrab_core_db_t *db);

/* ============================================================================
 * Query / Search Operations (NoSQL-style)
 * ============================================================================ */
size_t fossil_bluecrab_core_find_keys(fossil_bluecrab_core_db_t *db,
                                      const char *pattern,
                                      char ***out_keys);

size_t fossil_bluecrab_core_find_entries(fossil_bluecrab_core_db_t *db,
                                         const char *pattern,
                                         fossil_bluecrab_core_entry_t **out_entries);

bool fossil_bluecrab_core_clear(fossil_bluecrab_core_db_t *db); // Remove all entries

/* ============================================================================
 * Merge / Diff Operations (Git-style)
 * ============================================================================ */
bool fossil_bluecrab_core_diff(fossil_bluecrab_core_db_t *db,
                               const char *commit_a,
                               const char *commit_b);

bool fossil_bluecrab_core_merge(fossil_bluecrab_core_db_t *db,
                                const char *source_commit,
                                const char *target_commit,
                                bool auto_resolve_conflicts);

/* ============================================================================
 * Tagging / Bookmarking Commits
 * ============================================================================ */
bool fossil_bluecrab_core_tag_commit(fossil_bluecrab_core_db_t *db,
                                     const char *commit_hash,
                                     const char *tag_name);

char *fossil_bluecrab_core_get_tagged_commit(fossil_bluecrab_core_db_t *db,
                                             const char *tag_name);

/* ============================================================================
 * Export / Import Operations
 * ============================================================================ */
bool fossil_bluecrab_core_export_fson(fossil_bluecrab_core_db_t *db,
                                      const char *filepath);

bool fossil_bluecrab_core_import_fson(fossil_bluecrab_core_db_t *db,
                                      const char *filepath);

/* ============================================================================
 * Entry Metadata / Extended Utilities
 * ============================================================================ */
bool fossil_bluecrab_core_set_metadata(fossil_bluecrab_core_db_t *db,
                                       const char *key,
                                       const char *metadata);

char *fossil_bluecrab_core_get_metadata(fossil_bluecrab_core_db_t *db,
                                        const char *key);

bool fossil_bluecrab_core_has_key(fossil_bluecrab_core_db_t *db,
                                  const char *key);

/* ============================================================================
 * Advanced Hash / Tamper Detection
 * ============================================================================ */
bool fossil_bluecrab_core_verify_entry(fossil_bluecrab_core_entry_t *entry);
bool fossil_bluecrab_core_verify_db(fossil_bluecrab_core_db_t *db);

/* ============================================================================
 * Utility / Debug
 * ============================================================================ */
void fossil_bluecrab_core_print_entry(fossil_bluecrab_core_entry_t *entry);
void fossil_bluecrab_core_print_db(fossil_bluecrab_core_db_t *db);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */
char *fossil_bluecrab_core_hash_entry(const fossil_bluecrab_core_entry_t *entry);
fossil_bluecrab_core_value_t fossil_bluecrab_core_value_copy(const fossil_bluecrab_core_value_t *value);
void fossil_bluecrab_core_value_free(fossil_bluecrab_core_value_t *value);

/* ============================================================================
 * Time and Type Awareness
 * ============================================================================ */
const char *fossil_bluecrab_core_type_to_string(fossil_bluecrab_core_type_t type);
fossil_bluecrab_core_type_t fossil_bluecrab_core_string_to_type(const char *type_str);

#ifdef __cplusplus
}
#include <string>
#include <memory>
#include <optional>
#include <vector>

namespace fossil {

namespace bluecrab {

/* ============================================================================
 * C++ Wrapper for Blue Crab Core
 * ----------------------------------------------------------------------------
 * Provides RAII, type-safe methods, and std::string integration.
 * Methods use snake_case naming but call the underlying C functions.
 * ============================================================================ */
class Core {
public:
    Core() {
        db_ = std::make_unique<fossil_bluecrab_core_db_t>();
        fossil_bluecrab_core_init(db_.get(), "./bluecrab_db");
    }

    explicit Core(const std::string& path) {
        db_ = std::make_unique<fossil_bluecrab_core_db_t>();
        fossil_bluecrab_core_init(db_.get(), path.c_str());
    }

    ~Core() {
        fossil_bluecrab_core_shutdown(db_.get());
    }

    /* =========================================================================
     * CRUD Operations
     * ======================================================================== */
    bool set(const std::string& key, const fossil_bluecrab_core_value_t& value) {
        return fossil_bluecrab_core_set(db_.get(), key.c_str(), value);
    }

    std::optional<fossil_bluecrab_core_value_t> get(const std::string& key) {
        fossil_bluecrab_core_value_t value;
        if (fossil_bluecrab_core_get(db_.get(), key.c_str(), &value)) {
            return value;
        }
        return std::nullopt;
    }

    bool remove(const std::string& key) {
        return fossil_bluecrab_core_delete(db_.get(), key.c_str());
    }

    /* =========================================================================
     * Git-like Operations
     * ======================================================================== */
    bool commit(const std::string& message) {
        return fossil_bluecrab_core_commit(db_.get(), message.c_str());
    }

    bool checkout(const std::string& commit_hash) {
        return fossil_bluecrab_core_checkout(db_.get(), commit_hash.c_str());
    }

    bool branch(const std::string& branch_name) {
        return fossil_bluecrab_core_branch(db_.get(), branch_name.c_str());
    }

    bool log() {
        return fossil_bluecrab_core_log(db_.get());
    }

    /* =========================================================================
     * Utility / Hash
     * ======================================================================== */
    std::string hash_entry(const fossil_bluecrab_core_entry_t& entry) {
        char* h = fossil_bluecrab_core_hash_entry(&entry);
        std::string hash = h ? h : "";
        free(h); // C function allocates memory
        return hash;
    }

    std::string type_to_string(fossil_bluecrab_core_type_t type) const {
        return fossil_bluecrab_core_type_to_string(type);
    }

    fossil_bluecrab_core_type_t string_to_type(const std::string& type_str) const {
        return fossil_bluecrab_core_string_to_type(type_str.c_str());
    }

private:
    std::unique_ptr<fossil_bluecrab_core_db_t> db_;
};

} // namespace bluecrab

} // namespace fossil

#endif

#endif /* FOSSIL_CRABDB_FRAMEWORK_H */
