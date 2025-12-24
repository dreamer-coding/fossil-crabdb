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

typedef struct {
    fossil_bluecrab_table_t **tables;
    size_t table_count;
    size_t capacity;
    // internal transaction/state tracking
    uint64_t last_hash;
    bool in_transaction;
} fossil_bluecrab_db_t;

// ======================================================
// Hashing & Tamper Protection
// ======================================================

uint64_t fossil_bluecrab_core_hash(const void *data, size_t len);
bool fossil_bluecrab_core_verify_integrity(fossil_bluecrab_db_t *db);

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
// Transaction & Batch Processing
// ======================================================

bool fossil_bluecrab_core_begin_transaction(fossil_bluecrab_db_t *db);
bool fossil_bluecrab_core_commit_transaction(fossil_bluecrab_db_t *db);
bool fossil_bluecrab_core_rollback_transaction(fossil_bluecrab_db_t *db);
bool fossil_bluecrab_core_batch_process(fossil_bluecrab_table_t *table, fossil_bluecrab_entry_t **entries, size_t count);

// ======================================================
// AI/Time/Type Awareness Utilities
// ======================================================

fossil_bluecrab_type_t fossil_bluecrab_core_infer_type(const void *value);
time_t fossil_bluecrab_core_current_time(void);

// ======================================================
// Hybrid SQL + Git-like Logic
// ======================================================

bool fossil_bluecrab_core_commit_snapshot(fossil_bluecrab_db_t *db);
bool fossil_bluecrab_core_checkout_snapshot(fossil_bluecrab_db_t *db, uint64_t snapshot_hash);

#ifdef __cplusplus
}

namespace fossil {

    namespace bluecrab {



        };

    } // namespace bluecrab

} // namespace fossil

#endif

#endif /* FOSSIL_CRABDB_FRAMEWORK_H */
