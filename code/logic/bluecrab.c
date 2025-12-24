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
#include "fossil/crabdb/bluecrab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * DATABASE LIFECYCLE
 * ============================================================================ */
fossil_bluecrab_core_db_t* fossil_bluecrab_core_create_db(const char *name) {
    if (!name) return NULL;

    fossil_bluecrab_core_db_t *db = (fossil_bluecrab_core_db_t*)calloc(1, sizeof(fossil_bluecrab_core_db_t));
    if (!db) return NULL;

    // Initialize name safely
    strncpy(db->name, name, sizeof(db->name) - 1);
    db->name[sizeof(db->name) - 1] = '\0';

    // Initialize table count
    db->table_count = 0;

    // Stronger hash salt: combine time + pointer address
    uintptr_t addr = (uintptr_t)db;
    db->hash_salt = ((uint64_t)time(NULL) << 32) ^ (addr & 0xFFFFFFFF);

    // Timestamp
    db->last_modified = time(NULL);

    // Initialize table slots to NULL / zero
    for (size_t i = 0; i < FBC_MAX_TABLES; i++) {
        db->tables[i].row_count = 0;
        db->tables[i].column_count = 0;
        db->tables[i].rows = NULL;
        memset(db->tables[i].name, 0, sizeof(db->tables[i].name));
        for (size_t c = 0; c < FBC_MAX_COLUMNS; c++) {
            memset(db->tables[i].columns[c].name, 0, sizeof(db->tables[i].columns[c].name));
            db->tables[i].columns[c].type = FBC_TYPE_NULL;
            db->tables[i].columns[c].is_primary_key = false;
            db->tables[i].columns[c].is_indexed = false;
        }
    }

    return db;
}

bool fossil_bluecrab_core_destroy_db(fossil_bluecrab_core_db_t *db) {
    if (!db) return false;
    for (size_t i = 0; i < db->table_count; i++) {
        fossil_bluecrab_core_table_t *table = &db->tables[i];
        for (size_t r = 0; r < table->row_count; r++) {
            free(table->rows[r]);
        }
        free(table->rows);
    }
    free(db);
    return true;
}

/* ============================================================================
 * TABLE MANAGEMENT
 * ============================================================================ */
fossil_bluecrab_core_table_t* fossil_bluecrab_core_create_table(fossil_bluecrab_core_db_t *db, const char *table_name) {
    if (!db || !table_name || db->table_count >= FBC_MAX_TABLES) return NULL;
    fossil_bluecrab_core_table_t *table = &db->tables[db->table_count++];
    strncpy(table->name, table_name, sizeof(table->name)-1);
    table->row_count = 0;
    table->column_count = 0;
    table->rows = NULL;
    db->last_modified = time(NULL);
    return table;
}

bool fossil_bluecrab_core_drop_table(fossil_bluecrab_core_db_t *db, const char *table_name) {
    if (!db || !table_name) return false;
    for (size_t i = 0; i < db->table_count; i++) {
        if (strcmp(db->tables[i].name, table_name) == 0) {
            fossil_bluecrab_core_table_t *table = &db->tables[i];
            for (size_t r = 0; r < table->row_count; r++) {
                free(table->rows[r]);
            }
            free(table->rows);
            // shift remaining tables
            for (size_t j = i; j < db->table_count-1; j++) {
                db->tables[j] = db->tables[j+1];
            }
            db->table_count--;
            db->last_modified = time(NULL);
            return true;
        }
    }
    return false;
}

/* ============================================================================
 * ROW OPERATIONS
 * ============================================================================ */
bool fossil_bluecrab_core_insert_row(fossil_bluecrab_core_table_t *table, fossil_bluecrab_core_value_t *values) {
    if (!table || !values) return false;
    fossil_bluecrab_core_value_t **new_rows = (fossil_bluecrab_core_value_t**)realloc(table->rows, sizeof(fossil_bluecrab_core_value_t*) * (table->row_count + 1));
    if (!new_rows) return false;
    table->rows = new_rows;
    fossil_bluecrab_core_value_t *row_copy = (fossil_bluecrab_core_value_t*)calloc(table->column_count, sizeof(fossil_bluecrab_core_value_t));
    if (!row_copy) return false;
    memcpy(row_copy, values, sizeof(fossil_bluecrab_core_value_t) * table->column_count);
    table->rows[table->row_count++] = row_copy;
    return true;
}

bool fossil_bluecrab_core_delete_row(fossil_bluecrab_core_table_t *table, size_t row_index) {
    if (!table || row_index >= table->row_count) return false;
    free(table->rows[row_index]);
    for (size_t i = row_index; i < table->row_count - 1; i++) {
        table->rows[i] = table->rows[i+1];
    }
    table->row_count--;
    table->rows = (fossil_bluecrab_core_value_t**)realloc(table->rows, sizeof(fossil_bluecrab_core_value_t*) * table->row_count);
    return true;
}

fossil_bluecrab_core_value_t* fossil_bluecrab_core_get_value(fossil_bluecrab_core_table_t *table, size_t row_index, const char *column_name) {
    if (!table || row_index >= table->row_count || !column_name) return NULL;
    for (size_t c = 0; c < table->column_count; c++) {
        if (strcmp(table->columns[c].name, column_name) == 0) {
            return &table->rows[row_index][c];
        }
    }
    return NULL;
}

/* ============================================================================
 * SIMPLE HASH / TAMPER PROTECTION
 * ============================================================================ */
uint64_t fossil_bluecrab_core_hash(const void *data, size_t len, uint64_t salt) {
    const uint8_t *bytes = (const uint8_t*)data;
    uint64_t hash = salt ^ 0xcbf29ce484222325;
    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= 0x100000001b3;
    }
    return hash;
}

bool fossil_bluecrab_core_verify_hash(fossil_bluecrab_core_db_t *db) {
    if (!db) return false;

    uint64_t h = fossil_bluecrab_core_hash(db->name, strlen(db->name), db->hash_salt);
    h ^= fossil_bluecrab_core_hash(&db->table_count, sizeof(db->table_count), db->hash_salt);
    h ^= fossil_bluecrab_core_hash(&db->last_modified, sizeof(db->last_modified), db->hash_salt);

    // Walk all tables
    for (size_t t = 0; t < db->table_count; t++) {
        fossil_bluecrab_core_table_t *table = &db->tables[t];
        h ^= fossil_bluecrab_core_hash(table->name, strlen(table->name), db->hash_salt);
        h ^= fossil_bluecrab_core_hash(&table->column_count, sizeof(table->column_count), db->hash_salt);
        h ^= fossil_bluecrab_core_hash(&table->row_count, sizeof(table->row_count), db->hash_salt);

        // Columns
        for (size_t c = 0; c < table->column_count; c++) {
            fossil_bluecrab_core_column_t *col = &table->columns[c];
            h ^= fossil_bluecrab_core_hash(col->name, strlen(col->name), db->hash_salt);
            h ^= fossil_bluecrab_core_hash(&col->type, sizeof(col->type), db->hash_salt);
            h ^= fossil_bluecrab_core_hash(&col->is_primary_key, sizeof(col->is_primary_key), db->hash_salt);
            h ^= fossil_bluecrab_core_hash(&col->is_indexed, sizeof(col->is_indexed), db->hash_salt);
        }

        // Rows
        for (size_t r = 0; r < table->row_count; r++) {
            fossil_bluecrab_core_value_t *row = table->rows[r];
            for (size_t c = 0; c < table->column_count; c++) {
                fossil_bluecrab_core_value_t *val = &row[c];
                h ^= fossil_bluecrab_core_hash(&val->type, sizeof(val->type), db->hash_salt);
                switch (val->type) {
                    case FBC_TYPE_I8:     h ^= fossil_bluecrab_core_hash(&val->value.i8, sizeof(val->value.i8), db->hash_salt); break;
                    case FBC_TYPE_I16:    h ^= fossil_bluecrab_core_hash(&val->value.i16, sizeof(val->value.i16), db->hash_salt); break;
                    case FBC_TYPE_I32:    h ^= fossil_bluecrab_core_hash(&val->value.i32, sizeof(val->value.i32), db->hash_salt); break;
                    case FBC_TYPE_I64:    h ^= fossil_bluecrab_core_hash(&val->value.i64, sizeof(val->value.i64), db->hash_salt); break;
                    case FBC_TYPE_U8:     h ^= fossil_bluecrab_core_hash(&val->value.u8, sizeof(val->value.u8), db->hash_salt); break;
                    case FBC_TYPE_U16:    h ^= fossil_bluecrab_core_hash(&val->value.u16, sizeof(val->value.u16), db->hash_salt); break;
                    case FBC_TYPE_U32:    h ^= fossil_bluecrab_core_hash(&val->value.u32, sizeof(val->value.u32), db->hash_salt); break;
                    case FBC_TYPE_U64:    h ^= fossil_bluecrab_core_hash(&val->value.u64, sizeof(val->value.u64), db->hash_salt); break;
                    case FBC_TYPE_F32:    h ^= fossil_bluecrab_core_hash(&val->value.f32, sizeof(val->value.f32), db->hash_salt); break;
                    case FBC_TYPE_F64:    h ^= fossil_bluecrab_core_hash(&val->value.f64, sizeof(val->value.f64), db->hash_salt); break;
                    case FBC_TYPE_CSTR:   if(val->value.cstr) h ^= fossil_bluecrab_core_hash(val->value.cstr, strlen(val->value.cstr), db->hash_salt); break;
                    case FBC_TYPE_CHAR:   h ^= fossil_bluecrab_core_hash(&val->value.chr, sizeof(val->value.chr), db->hash_salt); break;
                    case FBC_TYPE_BOOL:   h ^= fossil_bluecrab_core_hash(&val->value.boolean, sizeof(val->value.boolean), db->hash_salt); break;
                    case FBC_TYPE_SIZE:   h ^= fossil_bluecrab_core_hash(&val->value.size, sizeof(val->value.size), db->hash_salt); break;
                    case FBC_TYPE_DATETIME: h ^= fossil_bluecrab_core_hash(&val->value.datetime, sizeof(val->value.datetime), db->hash_salt); break;
                    case FBC_TYPE_DURATION: h ^= fossil_bluecrab_core_hash(&val->value.duration, sizeof(val->value.duration), db->hash_salt); break;
                    case FBC_TYPE_ANY:    h ^= fossil_bluecrab_core_hash(&val->value.any, sizeof(val->value.any), db->hash_salt); break;
                    case FBC_TYPE_NULL:   break;
                    default: break;
                }
            }
        }
    }

    // Return true if hash is non-zero (indicates structure exists & not trivially empty)
    return h != 0;
}

/* ============================================================================
 * AI HOOK
 * ============================================================================ */

void fossil_bluecrab_core_ai_optimize(fossil_bluecrab_core_db_t *db) {
    if (!db) return;

    printf("[BlueCrab AI] Starting optimization for database '%s'\n", db->name);

    for (size_t t = 0; t < db->table_count; t++) {
        fossil_bluecrab_core_table_t *table = &db->tables[t];

        // Skip empty tables
        if (table->row_count == 0) {
            printf("  Table '%s' is empty, consider dropping if unused.\n", table->name);
            continue;
        }

        // Check for columns with identical values
        for (size_t c = 0; c < table->column_count; c++) {
            bool identical = true;
            fossil_bluecrab_core_value_t *first_val = &table->rows[0][c];

            for (size_t r = 1; r < table->row_count; r++) {
                fossil_bluecrab_core_value_t *val = &table->rows[r][c];
                if (val->type != first_val->type) {
                    identical = false;
                    break;
                }

                // Basic comparison by type
                switch (val->type) {
                    case FBC_TYPE_I8: identical &= (val->value.i8 == first_val->value.i8); break;
                    case FBC_TYPE_I16: identical &= (val->value.i16 == first_val->value.i16); break;
                    case FBC_TYPE_I32: identical &= (val->value.i32 == first_val->value.i32); break;
                    case FBC_TYPE_I64: identical &= (val->value.i64 == first_val->value.i64); break;
                    case FBC_TYPE_U8: identical &= (val->value.u8 == first_val->value.u8); break;
                    case FBC_TYPE_U16: identical &= (val->value.u16 == first_val->value.u16); break;
                    case FBC_TYPE_U32: identical &= (val->value.u32 == first_val->value.u32); break;
                    case FBC_TYPE_U64: identical &= (val->value.u64 == first_val->value.u64); break;
                    case FBC_TYPE_F32: identical &= (val->value.f32 == first_val->value.f32); break;
                    case FBC_TYPE_F64: identical &= (val->value.f64 == first_val->value.f64); break;
                    case FBC_TYPE_BOOL: identical &= (val->value.boolean == first_val->value.boolean); break;
                    case FBC_TYPE_CSTR: identical &= (val->value.cstr && first_val->value.cstr && strcmp(val->value.cstr, first_val->value.cstr) == 0); break;
                    case FBC_TYPE_CHAR: identical &= (val->value.chr == first_val->value.chr); break;
                    default: identical = false; break;
                }

                if (!identical) break;
            }

            if (identical) {
                printf("    Column '%s' has identical values in all rows. Consider compressing or indexing.\n",
                       table->columns[c].name);
            }
        }
    }

    printf("[BlueCrab AI] Optimization analysis complete.\n");
}
