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
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Type Definitions
 * ============================================================================ */

typedef enum {
    FBC_TYPE_I8, FBC_TYPE_I16, FBC_TYPE_I32, FBC_TYPE_I64,
    FBC_TYPE_U8, FBC_TYPE_U16, FBC_TYPE_U32, FBC_TYPE_U64,
    FBC_TYPE_F32, FBC_TYPE_F64,
    FBC_TYPE_CSTR, FBC_TYPE_CHAR,
    FBC_TYPE_BOOL,
    FBC_TYPE_HEX, FBC_TYPE_OCT, FBC_TYPE_BIN, FBC_TYPE_SIZE,
    FBC_TYPE_DATETIME, FBC_TYPE_DURATION,
    FBC_TYPE_ANY, FBC_TYPE_NULL
} fossil_bluecrab_core_type_t;

/* Generic value holder */
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
        char     c;         /* Single char */
        char    *cstr;      /* Null-terminated string */
        bool     b;
        uint64_t hex;
        uint64_t oct;
        uint64_t bin;
        size_t   size;
        time_t   datetime;
        double   duration;
        void    *any;       /* Generic pointer */
    } value;
} fossil_bluecrab_core_value_t;

/* ============================================================================
 * Record & Table Definitions
 * ============================================================================ */

typedef struct fossil_bluecrab_core_field {
    char name[64];
    fossil_bluecrab_core_type_t type;
    fossil_bluecrab_core_value_t default_value;
} fossil_bluecrab_core_field_t;

typedef struct fossil_bluecrab_core_record {
    size_t id;                      /* Auto-increment ID */
    fossil_bluecrab_core_value_t *values; /* Array of field values */
    size_t value_count;
    time_t created_at;
    time_t updated_at;
    uint64_t hash;                  /* Tamper protection */
} fossil_bluecrab_core_record_t;

typedef struct fossil_bluecrab_core_table {
    char name[64];
    fossil_bluecrab_core_field_t *fields;
    size_t field_count;
    fossil_bluecrab_core_record_t *records;
    size_t record_count;
} fossil_bluecrab_core_table_t;

/* ============================================================================
 * Database Core
 * ============================================================================ */

typedef struct fossil_bluecrab_core_db {
    fossil_bluecrab_core_table_t *tables;
    size_t table_count;
    bool in_transaction;
} fossil_bluecrab_core_db_t;

/* ============================================================================
 * Hashing / Tamper Protection
 * ============================================================================ */

/* Advanced hash function for records */
uint64_t fossil_bluecrab_core_hash_record(fossil_bluecrab_core_record_t *record);

/* ============================================================================
 * Initialization / Persistence
 * ============================================================================ */

void fossil_bluecrab_core_init(fossil_bluecrab_core_db_t *db);
void fossil_bluecrab_core_free(fossil_bluecrab_core_db_t *db);

/* Save/load database to persistent storage */
bool fossil_bluecrab_core_save(fossil_bluecrab_core_db_t *db, const char *filename);
bool fossil_bluecrab_core_load(fossil_bluecrab_core_db_t *db, const char *filename);

/* ============================================================================
 * CRUD Operations
 * ============================================================================ */

/* Table management */
bool fossil_bluecrab_core_create_table(fossil_bluecrab_core_db_t *db,
                                       const char *name,
                                       fossil_bluecrab_core_field_t *fields,
                                       size_t field_count);

bool fossil_bluecrab_core_drop_table(fossil_bluecrab_core_db_t *db,
                                     const char *name);

/* Record operations */
fossil_bluecrab_core_record_t* fossil_bluecrab_core_insert(fossil_bluecrab_core_db_t *db,
                                                          const char *table_name,
                                                          fossil_bluecrab_core_value_t *values,
                                                          size_t value_count);

bool fossil_bluecrab_core_update(fossil_bluecrab_core_db_t *db,
                                 const char *table_name,
                                 size_t record_id,
                                 fossil_bluecrab_core_value_t *values,
                                 size_t value_count);

bool fossil_bluecrab_core_delete(fossil_bluecrab_core_db_t *db,
                                 const char *table_name,
                                 size_t record_id);

fossil_bluecrab_core_record_t* fossil_bluecrab_core_select(fossil_bluecrab_core_db_t *db,
                                                           const char *table_name,
                                                           const char *query); /* hybrid query language */

/* ============================================================================
 * Transaction Processing
 * ============================================================================ */

bool fossil_bluecrab_core_begin_transaction(fossil_bluecrab_core_db_t *db);
bool fossil_bluecrab_core_commit_transaction(fossil_bluecrab_core_db_t *db);
bool fossil_bluecrab_core_rollback_transaction(fossil_bluecrab_core_db_t *db);

/* ============================================================================
 * Query & AI Helpers
 * ============================================================================ */

/* Evaluate hybrid SQL/Python/Meson-like query */
fossil_bluecrab_core_record_t* fossil_bluecrab_core_query(fossil_bluecrab_core_db_t *db,
                                                          const char *table_name,
                                                          const char *query);

/* AI-powered type inference & suggestion */
fossil_bluecrab_core_type_t fossil_bluecrab_core_infer_type(const char *value_str);

/* Time-aware operations */
time_t fossil_bluecrab_core_now();

/* ============================================================================
 * Utilities
 * ============================================================================ */

void fossil_bluecrab_core_print_record(fossil_bluecrab_core_record_t *record);
void fossil_bluecrab_core_print_table(fossil_bluecrab_core_table_t *table);

#ifdef __cplusplus
}

namespace fossil {

    namespace bluecrab {



        };

    } // namespace bluecrab

} // namespace fossil

#endif

#endif /* FOSSIL_CRABDB_FRAMEWORK_H */
