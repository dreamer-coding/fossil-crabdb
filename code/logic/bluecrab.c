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

// ======================================================
// Utilities
// ======================================================

static void* fossil_bluecrab_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "BlueCrab: Out of memory\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static char* fossil_bluecrab_strdup(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *copy = fossil_bluecrab_malloc(len);
    memcpy(copy, str, len);
    return copy;
}

// ======================================================
// Hashing & Tamper Protection
// ======================================================

uint64_t fossil_bluecrab_core_hash(const void *data, size_t len) {
    // Simple FNV-1a hash
    const uint8_t *ptr = (const uint8_t*)data;
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++) {
        hash ^= ptr[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

bool fossil_bluecrab_core_verify_integrity(fossil_bluecrab_db_t *db) {
    if (!db) return false;
    uint64_t hash_accum = 0;
    for (size_t i = 0; i < db->table_count; i++) {
        fossil_bluecrab_table_t *table = db->tables[i];
        for (size_t j = 0; j < table->entry_count; j++) {
            fossil_bluecrab_entry_t *entry = table->entries[j];
            hash_accum ^= fossil_bluecrab_core_hash(entry->key, strlen(entry->key));
            if (entry->value && entry->type != FBC_TYPE_NULL) {
                hash_accum ^= fossil_bluecrab_core_hash(entry->value, sizeof(entry->value));
            }
        }
    }
    return (hash_accum == db->last_hash);
}

// ======================================================
// Database Lifecycle
// ======================================================

fossil_bluecrab_db_t* fossil_bluecrab_core_create_db(size_t initial_capacity) {
    fossil_bluecrab_db_t *db = fossil_bluecrab_malloc(sizeof(fossil_bluecrab_db_t));
    db->tables = fossil_bluecrab_malloc(sizeof(fossil_bluecrab_table_t*) * initial_capacity);
    db->table_count = 0;
    db->capacity = initial_capacity;
    db->last_hash = 0;
    db->in_transaction = false;

    db->snapshots = fossil_bluecrab_malloc(sizeof(fossil_bluecrab_snapshot_t*) * 16);
    db->snapshot_count = 0;
    db->snapshot_capacity = 16;
    return db;
}

void fossil_bluecrab_core_free_db(fossil_bluecrab_db_t *db) {
    if (!db) return;
    for (size_t i = 0; i < db->table_count; i++) {
        fossil_bluecrab_table_t *table = db->tables[i];
        for (size_t j = 0; j < table->entry_count; j++) {
            free(table->entries[j]->key);
            free(table->entries[j]->value);
            free(table->entries[j]);
        }
        free(table->entries);
        free(table->name);
        free(table);
    }
    for (size_t i = 0; i < db->snapshot_count; i++) {
        free(db->snapshots[i]->message);
        free(db->snapshots[i]);
    }
    free(db->snapshots);
    free(db->tables);
    free(db);
}

// ======================================================
// Table Management
// ======================================================

fossil_bluecrab_table_t* fossil_bluecrab_core_create_table(fossil_bluecrab_db_t *db, const char *name) {
    if (!db || !name) return NULL;
    if (db->table_count >= db->capacity) {
        db->capacity *= 2;
        db->tables = realloc(db->tables, sizeof(fossil_bluecrab_table_t*) * db->capacity);
    }
    fossil_bluecrab_table_t *table = fossil_bluecrab_malloc(sizeof(fossil_bluecrab_table_t));
    table->name = fossil_bluecrab_strdup(name);
    table->entries = fossil_bluecrab_malloc(sizeof(fossil_bluecrab_entry_t*) * 16);
    table->entry_count = 0;
    table->capacity = 16;
    db->tables[db->table_count++] = table;
    return table;
}

bool fossil_bluecrab_core_drop_table(fossil_bluecrab_db_t *db, const char *name) {
    if (!db || !name) return false;
    for (size_t i = 0; i < db->table_count; i++) {
        if (strcmp(db->tables[i]->name, name) == 0) {
            fossil_bluecrab_table_t *table = db->tables[i];
            for (size_t j = 0; j < table->entry_count; j++) {
                free(table->entries[j]->key);
                free(table->entries[j]->value);
                free(table->entries[j]);
            }
            free(table->entries);
            free(table->name);
            free(table);
            // shift remaining tables
            for (size_t k = i; k < db->table_count - 1; k++)
                db->tables[k] = db->tables[k + 1];
            db->table_count--;
            return true;
        }
    }
    return false;
}

// ======================================================
// CRUD Operations
// ======================================================

bool fossil_bluecrab_core_insert(fossil_bluecrab_table_t *table, const char *key, void *value, fossil_bluecrab_type_t type) {
    if (!table || !key) return false;
    if (table->entry_count >= table->capacity) {
        table->capacity *= 2;
        table->entries = realloc(table->entries, sizeof(fossil_bluecrab_entry_t*) * table->capacity);
    }
    fossil_bluecrab_entry_t *entry = fossil_bluecrab_malloc(sizeof(fossil_bluecrab_entry_t));
    entry->key = fossil_bluecrab_strdup(key);
    entry->value = value ? value : NULL;
    entry->type = type;
    entry->created_at = time(NULL);
    entry->updated_at = entry->created_at;
    table->entries[table->entry_count++] = entry;
    return true;
}

fossil_bluecrab_entry_t* fossil_bluecrab_core_get(fossil_bluecrab_table_t *table, const char *key) {
    if (!table || !key) return NULL;
    for (size_t i = 0; i < table->entry_count; i++) {
        if (strcmp(table->entries[i]->key, key) == 0)
            return table->entries[i];
    }
    return NULL;
}

bool fossil_bluecrab_core_update(fossil_bluecrab_table_t *table, const char *key, void *value, fossil_bluecrab_type_t type) {
    fossil_bluecrab_entry_t *entry = fossil_bluecrab_core_get(table, key);
    if (!entry) return false;
    free(entry->value);
    entry->value = value;
    entry->type = type;
    entry->updated_at = time(NULL);
    return true;
}

bool fossil_bluecrab_core_delete(fossil_bluecrab_table_t *table, const char *key) {
    if (!table || !key) return false;
    for (size_t i = 0; i < table->entry_count; i++) {
        if (strcmp(table->entries[i]->key, key) == 0) {
            free(table->entries[i]->key);
            free(table->entries[i]->value);
            free(table->entries[i]);
            for (size_t j = i; j < table->entry_count - 1; j++)
                table->entries[j] = table->entries[j + 1];
            table->entry_count--;
            return true;
        }
    }
    return false;
}

// ======================================================
// Batch Processing & Query
// ======================================================

size_t fossil_bluecrab_core_query(fossil_bluecrab_table_t *table, fossil_bluecrab_filter_fn filter, void *userdata, fossil_bluecrab_entry_t **results, size_t max_results) {
    if (!table || !filter || !results) return 0;
    size_t found = 0;
    for (size_t i = 0; i < table->entry_count && found < max_results; i++) {
        if (filter(table->entries[i], userdata))
            results[found++] = table->entries[i];
    }
    return found;
}

bool fossil_bluecrab_core_batch_process(fossil_bluecrab_table_t *table, fossil_bluecrab_entry_t **entries, size_t count) {
    if (!table || !entries) return false;
    for (size_t i = 0; i < count; i++) {
        fossil_bluecrab_core_insert(table, entries[i]->key, entries[i]->value, entries[i]->type);
    }
    return true;
}

// ======================================================
// Transaction Management
// ======================================================

bool fossil_bluecrab_core_begin_transaction(fossil_bluecrab_db_t *db) {
    if (!db || db->in_transaction) return false;
    db->in_transaction = true;
    return true;
}

bool fossil_bluecrab_core_commit_transaction(fossil_bluecrab_db_t *db) {
    if (!db || !db->in_transaction) return false;
    db->in_transaction = false;
    db->last_hash = 0; // could recalc full db hash here
    return true;
}

bool fossil_bluecrab_core_rollback_transaction(fossil_bluecrab_db_t *db) {
    if (!db || !db->in_transaction) return false;
    db->in_transaction = false;
    return true;
}

// ======================================================
// AI / Time / Type Awareness Utilities
// ======================================================

fossil_bluecrab_type_t fossil_bluecrab_core_infer_type(const void *value) {
    // simple placeholder inference
    if (!value) return FBC_TYPE_NULL;
    return FBC_TYPE_ANY;
}

time_t fossil_bluecrab_core_current_time(void) {
    return time(NULL);
}

// ======================================================
// Hybrid SQL + Git-like Snapshot Logic
// ======================================================

bool fossil_bluecrab_core_commit_snapshot(fossil_bluecrab_db_t *db, const char *message) {
    if (!db || !message) return false;
    if (db->snapshot_count >= db->snapshot_capacity) {
        db->snapshot_capacity *= 2;
        db->snapshots = realloc(db->snapshots, sizeof(fossil_bluecrab_snapshot_t*) * db->snapshot_capacity);
    }
    fossil_bluecrab_snapshot_t *snap = fossil_bluecrab_malloc(sizeof(fossil_bluecrab_snapshot_t));
    snap->hash = db->last_hash; // placeholder, real hash should compute full DB
    snap->timestamp = time(NULL);
    snap->message = fossil_bluecrab_strdup(message);
    db->snapshots[db->snapshot_count++] = snap;
    return true;
}

bool fossil_bluecrab_core_checkout_snapshot(fossil_bluecrab_db_t *db, uint64_t snapshot_hash) {
    // Placeholder: real implementation would restore state
    for (size_t i = 0; i < db->snapshot_count; i++) {
        if (db->snapshots[i]->hash == snapshot_hash) {
            return true;
        }
    }
    return false;
}

// ======================================================
// Persistent Storage
// ======================================================

bool fossil_bluecrab_core_save(fossil_bluecrab_db_t *db, const char *filepath) {
    if (!db || !filepath) return false;
    FILE *f = fopen(filepath, "wb");
    if (!f) return false;
    fwrite(&db->table_count, sizeof(size_t), 1, f);
    // minimal: would need full table/entry serialization
    fclose(f);
    return true;
}

fossil_bluecrab_db_t* fossil_bluecrab_core_load(const char *filepath) {
    if (!filepath) return NULL;
    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;
    size_t table_count = 0;
    fread(&table_count, sizeof(size_t), 1, f);
    fclose(f);
    fossil_bluecrab_db_t *db = fossil_bluecrab_core_create_db(table_count);
    return db;
}
