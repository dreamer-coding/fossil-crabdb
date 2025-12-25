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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

/* ============================================================================
 * Utility: Duplicate String
 * ============================================================================ */
static char *fossil_bluecrab_core_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (copy) memcpy(copy, s, len + 1);
    return copy;
}

/* ============================================================================
 * Initialization / Shutdown
 * ============================================================================ */
bool fossil_bluecrab_core_init(fossil_bluecrab_core_db_t *db, const char *path) {
    if (!db || !path) return false;
    db->db_path = fossil_bluecrab_core_strdup(path);
    db->entry_count = 0;
    db->entries = NULL;
    db->current_commit = NULL;
    db->branch = fossil_bluecrab_core_strdup("main");
    return true;
}

bool fossil_bluecrab_core_shutdown(fossil_bluecrab_core_db_t *db) {
    if (!db) return false;

    for (size_t i = 0; i < db->entry_count; i++) {
        fossil_bluecrab_core_value_free(&db->entries[i].value);
        free(db->entries[i].key);
        free(db->entries[i].metadata);
        free(db->entries[i].hash);
    }
    free(db->entries);
    free(db->db_path);
    free(db->current_commit);
    free(db->branch);
    db->entries = NULL;
    db->entry_count = 0;
    return true;
}

/* ============================================================================
 * Utility: Value Copy and Free
 * ============================================================================ */
fossil_bluecrab_core_value_t fossil_bluecrab_core_value_copy(const fossil_bluecrab_core_value_t *value) {
    fossil_bluecrab_core_value_t copy;
    memset(&copy, 0, sizeof(copy));
    copy.type = value->type;

    switch (value->type) {
        case FBC_TYPE_CSTR:
        case FBC_TYPE_HEX:
        case FBC_TYPE_OCT:
        case FBC_TYPE_BIN:
            copy.value.cstr = fossil_bluecrab_core_strdup(value->value.cstr);
            break;
        case FBC_TYPE_CHAR:
            copy.value.ch = value->value.ch;
            break;
        case FBC_TYPE_BOOL:
            copy.value.boolean = value->value.boolean;
            break;
        case FBC_TYPE_I8: copy.value.i8 = value->value.i8; break;
        case FBC_TYPE_I16: copy.value.i16 = value->value.i16; break;
        case FBC_TYPE_I32: copy.value.i32 = value->value.i32; break;
        case FBC_TYPE_I64: copy.value.i64 = value->value.i64; break;
        case FBC_TYPE_U8: copy.value.u8 = value->value.u8; break;
        case FBC_TYPE_U16: copy.value.u16 = value->value.u16; break;
        case FBC_TYPE_U32: copy.value.u32 = value->value.u32; break;
        case FBC_TYPE_U64: copy.value.u64 = value->value.u64; break;
        case FBC_TYPE_F32: copy.value.f32 = value->value.f32; break;
        case FBC_TYPE_F64: copy.value.f64 = value->value.f64; break;
        case FBC_TYPE_SIZE: copy.value.size = value->value.size; break;
        case FBC_TYPE_DATETIME: copy.value.datetime = value->value.datetime; break;
        case FBC_TYPE_DURATION: copy.value.duration = value->value.duration; break;
        default: break;
    }

    return copy;
}

void fossil_bluecrab_core_value_free(fossil_bluecrab_core_value_t *value) {
    if (!value) return;
    switch (value->type) {
        case FBC_TYPE_CSTR:
        case FBC_TYPE_HEX:
        case FBC_TYPE_OCT:
        case FBC_TYPE_BIN:
            free(value->value.cstr);
            break;
        default: break;
    }
    value->type = FBC_TYPE_NULL;
}

/* ============================================================================
 * Hashing: Simple placeholder hash for entries (for tamper protection)
 * ============================================================================ */
char *fossil_bluecrab_core_hash_entry(const fossil_bluecrab_core_entry_t *entry) {
    if (!entry) return NULL;
    /* Very simple hash: sum of chars mod 256 as hex string (placeholder) */
    unsigned int sum = 0;
    for (size_t i = 0; entry->key[i]; i++) sum += (unsigned char)entry->key[i];
    char *hash = malloc(9);
    if (!hash) return NULL;
    snprintf(hash, 9, "%08X", sum);
    return hash;
}

/* ============================================================================
 * CRUD Operations
 * ============================================================================ */
bool fossil_bluecrab_core_set(fossil_bluecrab_core_db_t *db,
                              const char *key,
                              fossil_bluecrab_core_value_t value) {
    if (!db || !key) return false;

    /* Search existing entry */
    for (size_t i = 0; i < db->entry_count; i++) {
        if (strcmp(db->entries[i].key, key) == 0) {
            fossil_bluecrab_core_value_free(&db->entries[i].value);
            db->entries[i].value = fossil_bluecrab_core_value_copy(&value);
            db->entries[i].updated_at = time(NULL);
            free(db->entries[i].hash);
            db->entries[i].hash = fossil_bluecrab_core_hash_entry(&db->entries[i]);
            return true;
        }
    }

    /* New entry */
    fossil_bluecrab_core_entry_t *new_entries = realloc(db->entries, sizeof(fossil_bluecrab_core_entry_t) * (db->entry_count + 1));
    if (!new_entries) return false;

    db->entries = new_entries;
    fossil_bluecrab_core_entry_t *entry = &db->entries[db->entry_count];
    entry->key = fossil_bluecrab_core_strdup(key);
    entry->value = fossil_bluecrab_core_value_copy(&value);
    entry->created_at = entry->updated_at = time(NULL);
    entry->metadata = NULL;
    entry->hash = fossil_bluecrab_core_hash_entry(entry);
    db->entry_count++;
    return true;
}

bool fossil_bluecrab_core_get(fossil_bluecrab_core_db_t *db,
                              const char *key,
                              fossil_bluecrab_core_value_t *out_value) {
    if (!db || !key || !out_value) return false;

    for (size_t i = 0; i < db->entry_count; i++) {
        if (strcmp(db->entries[i].key, key) == 0) {
            *out_value = fossil_bluecrab_core_value_copy(&db->entries[i].value);
            return true;
        }
    }
    return false;
}

bool fossil_bluecrab_core_delete(fossil_bluecrab_core_db_t *db, const char *key) {
    if (!db || !key) return false;

    for (size_t i = 0; i < db->entry_count; i++) {
        if (strcmp(db->entries[i].key, key) == 0) {
            fossil_bluecrab_core_value_free(&db->entries[i].value);
            free(db->entries[i].key);
            free(db->entries[i].metadata);
            free(db->entries[i].hash);

            /* Shift remaining entries */
            for (size_t j = i; j < db->entry_count - 1; j++) {
                db->entries[j] = db->entries[j + 1];
            }
            db->entry_count--;
            db->entries = realloc(db->entries, sizeof(fossil_bluecrab_core_entry_t) * db->entry_count);
            return true;
        }
    }
    return false;
}

/* ============================================================================
 * Type Awareness
 * ============================================================================ */
const char *fossil_bluecrab_core_type_to_string(fossil_bluecrab_core_type_t type) {
    switch (type) {
        case FBC_TYPE_NULL: return "null";
        case FBC_TYPE_ANY: return "any";
        case FBC_TYPE_I8: return "i8";
        case FBC_TYPE_I16: return "i16";
        case FBC_TYPE_I32: return "i32";
        case FBC_TYPE_I64: return "i64";
        case FBC_TYPE_U8: return "u8";
        case FBC_TYPE_U16: return "u16";
        case FBC_TYPE_U32: return "u32";
        case FBC_TYPE_U64: return "u64";
        case FBC_TYPE_F32: return "f32";
        case FBC_TYPE_F64: return "f64";
        case FBC_TYPE_CSTR: return "cstr";
        case FBC_TYPE_CHAR: return "char";
        case FBC_TYPE_BOOL: return "bool";
        case FBC_TYPE_HEX: return "hex";
        case FBC_TYPE_OCT: return "oct";
        case FBC_TYPE_BIN: return "bin";
        case FBC_TYPE_SIZE: return "size";
        case FBC_TYPE_DATETIME: return "datetime";
        case FBC_TYPE_DURATION: return "duration";
        default: return "unknown";
    }
}

fossil_bluecrab_core_type_t fossil_bluecrab_core_string_to_type(const char *type_str) {
    if (!type_str) return FBC_TYPE_NULL;
    if (strcmp(type_str, "i8") == 0) return FBC_TYPE_I8;
    if (strcmp(type_str, "i16") == 0) return FBC_TYPE_I16;
    if (strcmp(type_str, "i32") == 0) return FBC_TYPE_I32;
    if (strcmp(type_str, "i64") == 0) return FBC_TYPE_I64;
    if (strcmp(type_str, "u8") == 0) return FBC_TYPE_U8;
    if (strcmp(type_str, "u16") == 0) return FBC_TYPE_U16;
    if (strcmp(type_str, "u32") == 0) return FBC_TYPE_U32;
    if (strcmp(type_str, "u64") == 0) return FBC_TYPE_U64;
    if (strcmp(type_str, "f32") == 0) return FBC_TYPE_F32;
    if (strcmp(type_str, "f64") == 0) return FBC_TYPE_F64;
    if (strcmp(type_str, "cstr") == 0) return FBC_TYPE_CSTR;
    if (strcmp(type_str, "char") == 0) return FBC_TYPE_CHAR;
    if (strcmp(type_str, "bool") == 0) return FBC_TYPE_BOOL;
    if (strcmp(type_str, "hex") == 0) return FBC_TYPE_HEX;
    if (strcmp(type_str, "oct") == 0) return FBC_TYPE_OCT;
    if (strcmp(type_str, "bin") == 0) return FBC_TYPE_BIN;
    if (strcmp(type_str, "size") == 0) return FBC_TYPE_SIZE;
    if (strcmp(type_str, "datetime") == 0) return FBC_TYPE_DATETIME;
    if (strcmp(type_str, "duration") == 0) return FBC_TYPE_DURATION;
    if (strcmp(type_str, "any") == 0) return FBC_TYPE_ANY;
    if (strcmp(type_str, "null") == 0) return FBC_TYPE_NULL;
    return FBC_TYPE_NULL;
}

/* ============================================================================
 * Git-like Operations
 * ============================================================================ */
bool fossil_bluecrab_core_log(fossil_bluecrab_core_db_t *db) {
    if (!db) return false;
    printf("[BlueCrab] Branch: %s, Current commit: %s\n",
           db->branch ? db->branch : "none",
           db->current_commit ? db->current_commit : "none");

    for (size_t i = 0; i < db->commit_count; i++) {
        printf("  %s - %s (%ld)\n",
               db->commits[i].hash,
               db->commits[i].message,
               db->commits[i].timestamp);
    }
    return true;
}

bool fossil_bluecrab_core_commit(fossil_bluecrab_core_db_t *db, const char *message) {
    if (!db || !message) return false;

    size_t snapshot_count;
    fossil_bluecrab_core_entry_t *snapshot = copy_entries(db, &snapshot_count);
    if (!snapshot && db->entry_count > 0) return false;

    fossil_bluecrab_core_commit_t *new_commits = realloc(db->commits, sizeof(fossil_bluecrab_core_commit_t) * (db->commit_count + 1));
    if (!new_commits) return false;
    db->commits = new_commits;

    fossil_bluecrab_core_commit_t *commit = &db->commits[db->commit_count];
    commit->snapshot = snapshot;
    commit->snapshot_count = snapshot_count;
    commit->timestamp = time(NULL);
    commit->message = fossil_bluecrab_core_strdup(message);

    /* Simple commit hash: "commit_X" */
    char hash[64];
    snprintf(hash, sizeof(hash), "commit_%zu", db->commit_count + 1);
    commit->hash = fossil_bluecrab_core_strdup(hash);

    free(db->current_commit);
    db->current_commit = fossil_bluecrab_core_strdup(hash);

    db->commit_count++;

    printf("[BlueCrab] Commit %s: %s\n", hash, message);
    return true;
}

bool fossil_bluecrab_core_checkout(fossil_bluecrab_core_db_t *db, const char *commit_hash) {
    if (!db || !commit_hash) return false;

    for (size_t i = 0; i < db->commit_count; i++) {
        if (strcmp(db->commits[i].hash, commit_hash) == 0) {
            /* Free current entries */
            for (size_t j = 0; j < db->entry_count; j++) {
                fossil_bluecrab_core_value_free(&db->entries[j].value);
                free(db->entries[j].key);
                free(db->entries[j].metadata);
                free(db->entries[j].hash);
            }
            free(db->entries);

            /* Load snapshot */
            size_t count = db->commits[i].snapshot_count;
            db->entries = copy_entries((fossil_bluecrab_core_db_t *)&db->commits[i], &count);
            db->entry_count = count;

            free(db->current_commit);
            db->current_commit = fossil_bluecrab_core_strdup(commit_hash);

            printf("[BlueCrab] Checked out commit: %s\n", commit_hash);
            return true;
        }
    }

    printf("[BlueCrab] Commit %s not found\n", commit_hash);
    return false;
}

bool fossil_bluecrab_core_branch(fossil_bluecrab_core_db_t *db, const char *branch_name) {
    if (!db || !branch_name) return false;

    free(db->branch);
    db->branch = fossil_bluecrab_core_strdup(branch_name);

    printf("[BlueCrab] Switched to branch: %s\n", branch_name);
    return true;
}

bool fossil_bluecrab_core_log(fossil_bluecrab_core_db_t *db) {
    if (!db) return false;
    printf("[BlueCrab] Branch: %s, Current commit: %s\n",
           db->branch ? db->branch : "none",
           db->current_commit ? db->current_commit : "none");

    for (size_t i = 0; i < db->commit_count; i++) {
        printf("  %s - %s (%ld)\n",
               db->commits[i].hash,
               db->commits[i].message,
               db->commits[i].timestamp);
    }
    return true;
}

bool fossil_bluecrab_core_save(fossil_bluecrab_core_db_t *db) {
    if (!db || !db->db_path) return false;

    char tmp_path[1024];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", db->db_path);

    FILE *f = fopen(tmp_path, "wb");
    if (!f) return false;

    /* Save entries */
    fwrite(&db->entry_count, sizeof(size_t), 1, f);
    for (size_t i = 0; i < db->entry_count; i++) {
        fossil_bluecrab_core_entry_t *e = &db->entries[i];
        size_t key_len = strlen(e->key) + 1;
        fwrite(&key_len, sizeof(size_t), 1, f);
        fwrite(e->key, 1, key_len, f);

        fwrite(&e->value.type, sizeof(fossil_bluecrab_core_type_t), 1, f);
        if (e->value.type == FBC_TYPE_I32)
            fwrite(&e->value.value.i32, sizeof(int32_t), 1, f);
        else if (e->value.type == FBC_TYPE_CSTR) {
            size_t len = strlen(e->value.value.cstr) + 1;
            fwrite(&len, sizeof(size_t), 1, f);
            fwrite(e->value.value.cstr, 1, len, f);
        }

        fwrite(&e->created_at, sizeof(time_t), 1, f);
        fwrite(&e->updated_at, sizeof(time_t), 1, f);

        size_t hash_len = e->hash ? strlen(e->hash) + 1 : 0;
        fwrite(&hash_len, sizeof(size_t), 1, f);
        if (hash_len > 0) fwrite(e->hash, 1, hash_len, f);
    }

    /* Save commits */
    fwrite(&db->commit_count, sizeof(size_t), 1, f);
    for (size_t i = 0; i < db->commit_count; i++) {
        fossil_bluecrab_core_commit_t *c = &db->commits[i];
        size_t hash_len = strlen(c->hash) + 1;
        fwrite(&hash_len, sizeof(size_t), 1, f);
        fwrite(c->hash, 1, hash_len, f);

        size_t msg_len = strlen(c->message) + 1;
        fwrite(&msg_len, sizeof(size_t), 1, f);
        fwrite(c->message, 1, msg_len, f);

        fwrite(&c->timestamp, sizeof(time_t), 1, f);

        /* Snapshot */
        fwrite(&c->snapshot_count, sizeof(size_t), 1, f);
        for (size_t j = 0; j < c->snapshot_count; j++) {
            fossil_bluecrab_core_entry_t *e = &c->snapshot[j];
            size_t key_len = strlen(e->key) + 1;
            fwrite(&key_len, sizeof(size_t), 1, f);
            fwrite(e->key, 1, key_len, f);
            fwrite(&e->value.type, sizeof(fossil_bluecrab_core_type_t), 1, f);
            if (e->value.type == FBC_TYPE_I32)
                fwrite(&e->value.value.i32, sizeof(int32_t), 1, f);
            else if (e->value.type == FBC_TYPE_CSTR) {
                size_t len = strlen(e->value.value.cstr) + 1;
                fwrite(&len, sizeof(size_t), 1, f);
                fwrite(e->value.value.cstr, 1, len, f);
            }
            fwrite(&e->created_at, sizeof(time_t), 1, f);
            fwrite(&e->updated_at, sizeof(time_t), 1, f);
            size_t hash_len = e->hash ? strlen(e->hash) + 1 : 0;
            fwrite(&hash_len, sizeof(size_t), 1, f);
            if (hash_len > 0) fwrite(e->hash, 1, hash_len, f);
        }
    }

    /* Save branch & current commit */
    size_t branch_len = strlen(db->branch) + 1;
    fwrite(&branch_len, sizeof(size_t), 1, f);
    fwrite(db->branch, 1, branch_len, f);

    size_t commit_len = db->current_commit ? strlen(db->current_commit) + 1 : 0;
    fwrite(&commit_len, sizeof(size_t), 1, f);
    if (commit_len > 0) fwrite(db->current_commit, 1, commit_len, f);

    fclose(f);

    /* Atomic rename */
    rename(tmp_path, db->db_path);
    return true;
}

bool fossil_bluecrab_core_load(fossil_bluecrab_core_db_t *db) {
    if (!db || !db->db_path) return false;
    FILE *f = fopen(db->db_path, "rb");
    if (!f) return false;

    fread(&db->entry_count, sizeof(size_t), 1, f);
    db->entries = malloc(sizeof(fossil_bluecrab_core_entry_t) * db->entry_count);
    for (size_t i = 0; i < db->entry_count; i++) {
        fossil_bluecrab_core_entry_t *e = &db->entries[i];
        size_t key_len;
        fread(&key_len, sizeof(size_t), 1, f);
        e->key = malloc(key_len);
        fread(e->key, 1, key_len, f);

        fread(&e->value.type, sizeof(fossil_bluecrab_core_type_t), 1, f);
        if (e->value.type == FBC_TYPE_I32)
            fread(&e->value.value.i32, sizeof(int32_t), 1, f);
        else if (e->value.type == FBC_TYPE_CSTR) {
            size_t len;
            fread(&len, sizeof(size_t), 1, f);
            e->value.value.cstr = malloc(len);
            fread(e->value.value.cstr, 1, len, f);
        }

        fread(&e->created_at, sizeof(time_t), 1, f);
        fread(&e->updated_at, sizeof(time_t), 1, f);

        size_t hash_len;
        fread(&hash_len, sizeof(size_t), 1, f);
        if (hash_len > 0) {
            e->hash = malloc(hash_len);
            fread(e->hash, 1, hash_len, f);
        } else e->hash = NULL;
    }

    /* Commits */
    fread(&db->commit_count, sizeof(size_t), 1, f);
    db->commits = malloc(sizeof(fossil_bluecrab_core_commit_t) * db->commit_count);
    for (size_t i = 0; i < db->commit_count; i++) {
        fossil_bluecrab_core_commit_t *c = &db->commits[i];

        size_t hash_len;
        fread(&hash_len, sizeof(size_t), 1, f);
        c->hash = malloc(hash_len);
        fread(c->hash, 1, hash_len, f);

        size_t msg_len;
        fread(&msg_len, sizeof(size_t), 1, f);
        c->message = malloc(msg_len);
        fread(c->message, 1, msg_len, f);

        fread(&c->timestamp, sizeof(time_t), 1, f);

        fread(&c->snapshot_count, sizeof(size_t), 1, f);
        c->snapshot = malloc(sizeof(fossil_bluecrab_core_entry_t) * c->snapshot_count);
        for (size_t j = 0; j < c->snapshot_count; j++) {
            fossil_bluecrab_core_entry_t *e = &c->snapshot[j];
            size_t key_len;
            fread(&key_len, sizeof(size_t), 1, f);
            e->key = malloc(key_len);
            fread(e->key, 1, key_len, f);

            fread(&e->value.type, sizeof(fossil_bluecrab_core_type_t), 1, f);
            if (e->value.type == FBC_TYPE_I32)
                fread(&e->value.value.i32, sizeof(int32_t), 1, f);
            else if (e->value.type == FBC_TYPE_CSTR) {
                size_t len;
                fread(&len, sizeof(size_t), 1, f);
                e->value.value.cstr = malloc(len);
                fread(e->value.value.cstr, 1, len, f);
            }

            fread(&e->created_at, sizeof(time_t), 1, f);
            fread(&e->updated_at, sizeof(time_t), 1, f);

            size_t hash_len;
            fread(&hash_len, sizeof(size_t), 1, f);
            if (hash_len > 0) {
                e->hash = malloc(hash_len);
                fread(e->hash, 1, hash_len, f);
            } else e->hash = NULL;
        }
    }

    /* Branch & current commit */
    size_t branch_len;
    fread(&branch_len, sizeof(size_t), 1, f);
    db->branch = malloc(branch_len);
    fread(db->branch, 1, branch_len, f);

    size_t commit_len;
    fread(&commit_len, sizeof(size_t), 1, f);
    if (commit_len > 0) {
        db->current_commit = malloc(commit_len);
        fread(db->current_commit, 1, commit_len, f);
    } else db->current_commit = NULL;

    fclose(f);
    return true;
}
