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
 *
 * Portable, multi-purpose, verbose core for Fossil Logic.
 * Features:
 * - Persistent data storage
 * - Cold-core tamper protection
 * - Type and time awareness
 * - AI-friendly operations
 * - Git-inspired hybrid database operations
 * - Verbose API for sub-libraries
 * ============================================================================ */

/* ----------------------------
 * Type Definitions
 * ---------------------------- */

typedef enum {
    FOSSIL_BLUECRAB_TYPE_NULL,
    FOSSIL_BLUECRAB_TYPE_BOOL,

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

    /* Strings/Text */
    FOSSIL_BLUECRAB_TYPE_CSTR,
    FOSSIL_BLUECRAB_TYPE_CHAR,

    /* Extended types */
    FOSSIL_BLUECRAB_TYPE_HEX,
    FOSSIL_BLUECRAB_TYPE_OCT,
    FOSSIL_BLUECRAB_TYPE_BIN,
    FOSSIL_BLUECRAB_TYPE_SIZE,
    FOSSIL_BLUECRAB_TYPE_DATETIME,
    FOSSIL_BLUECRAB_TYPE_DURATION,

    /* Generic */
    FOSSIL_BLUECRAB_TYPE_ANY
} fossil_bluecrab_core_type_t;

/* Generic data container */
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
        char     c;

        bool     boolean;

        /* Extended representations as strings for flexibility */
        char    *hex;
        char    *oct;
        char    *bin;
        size_t   size;
        time_t   datetime;
        double   duration;

        void    *any;
    } value;
} fossil_bluecrab_core_data_t;

/* ----------------------------
 * Database Structures
 * ---------------------------- */

/* Each record in the database */
typedef struct fossil_bluecrab_core_record_s {
    char *key;                          /* Unique key */
    fossil_bluecrab_core_data_t data;   /* Typed data */
    time_t timestamp;                   /* Last modification */
    struct fossil_bluecrab_core_record_s *next;
} fossil_bluecrab_core_record_t;

/* Database handle */
typedef struct {
    char *name;                         /* Database name */
    fossil_bluecrab_core_record_t *head;
    bool dirty;                         /* Tracks uncommitted changes */
} fossil_bluecrab_core_db_t;

/* ----------------------------
 * Core Database Functions
 * ---------------------------- */

/* Initialize database handle */
void fossil_bluecrab_core_init_db(fossil_bluecrab_core_db_t *db, const char *name);

/* Destroy database and free memory */
void fossil_bluecrab_core_destroy_db(fossil_bluecrab_core_db_t *db);

/* Add or update a record */
bool fossil_bluecrab_core_set(fossil_bluecrab_core_db_t *db, const char *key, fossil_bluecrab_core_data_t value);

/* Get a record by key */
bool fossil_bluecrab_core_get(fossil_bluecrab_core_db_t *db, const char *key, fossil_bluecrab_core_data_t *out_value);

/* Delete a record */
bool fossil_bluecrab_core_delete(fossil_bluecrab_core_db_t *db, const char *key);

/* List all keys */
size_t fossil_bluecrab_core_list_keys(fossil_bluecrab_core_db_t *db, char ***keys_out);

/* ----------------------------
 * Persistent Storage
 * ---------------------------- */

/* Save database to disk (persistent storage) */
bool fossil_bluecrab_core_save(fossil_bluecrab_core_db_t *db, const char *filepath);

/* Load database from disk */
bool fossil_bluecrab_core_load(fossil_bluecrab_core_db_t *db, const char *filepath);

/* ----------------------------
 * Cold-Core Tamper Protection
 * ---------------------------- */

/* Simple hash for tamper detection */
uint64_t fossil_bluecrab_core_hash_record(fossil_bluecrab_core_record_t *record);

/* Verify database integrity */
bool fossil_bluecrab_core_verify_integrity(fossil_bluecrab_core_db_t *db);

/* ----------------------------
 * Git-Inspired Operations
 * ---------------------------- */

/* Commit current state */
bool fossil_bluecrab_core_commit(fossil_bluecrab_core_db_t *db, const char *message);

/* Rollback to previous commit */
bool fossil_bluecrab_core_rollback(fossil_bluecrab_core_db_t *db, const char *commit_hash);

/* Show commit log */
void fossil_bluecrab_core_log(fossil_bluecrab_core_db_t *db);

/* ----------------------------
 * Utility & AI Tricks
 * ---------------------------- */

/* Print a record verbosely */
void fossil_bluecrab_core_print_record(fossil_bluecrab_core_record_t *record);

/* Infer type from generic data (AI-like type awareness) */
fossil_bluecrab_core_type_t fossil_bluecrab_core_infer_type(void *data, size_t size);

/* Time-aware operations */
time_t fossil_bluecrab_core_current_time(void);

#ifdef __cplusplus
}
#include <string>
#include <vector>
#include <memory>

namespace fossil {

namespace bluecrab {

class Core {
public:
    /* ----------------------------
     * Constructor / Destructor
     * ---------------------------- */
    Core(const std::string &db_name);
    ~Core();

    /* ----------------------------
     * Core Database Operations
     * ---------------------------- */
    bool init_db();
    void destroy_db();

    bool set(const std::string &key, const fossil_bluecrab_core_data_t &value);
    bool get(const std::string &key, fossil_bluecrab_core_data_t &out_value);
    bool delete_record(const std::string &key);
    std::vector<std::string> list_keys();

    /* ----------------------------
     * Persistent Storage
     * ---------------------------- */
    bool save(const std::string &filepath);
    bool load(const std::string &filepath);

    /* ----------------------------
     * Tamper Protection
     * ---------------------------- */
    uint64_t hash_record(fossil_bluecrab_core_record_t *record);
    bool verify_integrity();

    /* ----------------------------
     * Git-Inspired Operations
     * ---------------------------- */
    bool commit(const std::string &message);
    bool rollback(const std::string &commit_hash);
    void log();

    /* ----------------------------
     * Utility / AI Tricks
     * ---------------------------- */
    void print_record(fossil_bluecrab_core_record_t *record);
    fossil_bluecrab_core_type_t infer_type(void *data, size_t size);
    std::time_t current_time();

private:
    fossil_bluecrab_core_db_t db_; // The underlying C database handle
    std::string db_name_;
};

} // namespace bluecrab

} // namespace fossil

#endif

#endif /* FOSSIL_CRABDB_FRAMEWORK_H */
