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
    if (!entry || !entry->key) return NULL;

    /* 64-bit FNV-1a base */
    uint64_t hash = 1469598103934665603ULL;

    /* Mix helper */
    #define MIX_BYTE(b) do { \
        hash ^= (uint64_t)(unsigned char)(b); \
        hash *= 1099511628211ULL; \
    } while (0)

    /* Key */
    for (const char *p = entry->key; *p; ++p)
        MIX_BYTE(*p);

    /* Type */
    MIX_BYTE(entry->value.type & 0xFF);
    MIX_BYTE((entry->value.type >> 8) & 0xFF);

    /* Value (type-aware) */
    switch (entry->value.type) {
        case FBC_TYPE_I8:   MIX_BYTE(entry->value.value.i8); break;
        case FBC_TYPE_I16:  for (int i = 0; i < 2; i++) MIX_BYTE(entry->value.value.i16 >> (i * 8)); break;
        case FBC_TYPE_I32:  for (int i = 0; i < 4; i++) MIX_BYTE(entry->value.value.i32 >> (i * 8)); break;
        case FBC_TYPE_I64:  for (int i = 0; i < 8; i++) MIX_BYTE(entry->value.value.i64 >> (i * 8)); break;

        case FBC_TYPE_U8:   MIX_BYTE(entry->value.value.u8); break;
        case FBC_TYPE_U16:  for (int i = 0; i < 2; i++) MIX_BYTE(entry->value.value.u16 >> (i * 8)); break;
        case FBC_TYPE_U32:  for (int i = 0; i < 4; i++) MIX_BYTE(entry->value.value.u32 >> (i * 8)); break;
        case FBC_TYPE_U64:  for (int i = 0; i < 8; i++) MIX_BYTE(entry->value.value.u64 >> (i * 8)); break;

        case FBC_TYPE_F32: {
            uint32_t v;
            memcpy(&v, &entry->value.value.f32, sizeof(v));
            for (int i = 0; i < 4; i++) MIX_BYTE(v >> (i * 8));
            break;
        }

        case FBC_TYPE_F64: {
            uint64_t v;
            memcpy(&v, &entry->value.value.f64, sizeof(v));
            for (int i = 0; i < 8; i++) MIX_BYTE(v >> (i * 8));
            break;
        }

        case FBC_TYPE_CSTR:
            if (entry->value.value.cstr)
                for (const char *p = entry->value.value.cstr; *p; ++p)
                    MIX_BYTE(*p);
            break;

        case FBC_TYPE_CHAR:
            MIX_BYTE(entry->value.value.ch);
            break;

        case FBC_TYPE_BOOL:
            MIX_BYTE(entry->value.value.boolean ? 1 : 0);
            break;

        case FBC_TYPE_SIZE:
            for (int i = 0; i < sizeof(size_t); i++)
                MIX_BYTE(entry->value.value.size >> (i * 8));
            break;

        case FBC_TYPE_DATETIME:
            for (int i = 0; i < sizeof(time_t); i++)
                MIX_BYTE(entry->value.value.datetime >> (i * 8));
            break;

        case FBC_TYPE_DURATION: {
            uint64_t v;
            memcpy(&v, &entry->value.value.duration, sizeof(v));
            for (int i = 0; i < 8; i++) MIX_BYTE(v >> (i * 8));
            break;
        }

        default:
            break;
    }

    /* Metadata */
    if (entry->metadata) {
        for (const char *p = entry->metadata; *p; ++p)
            MIX_BYTE(*p);
    }

    /* Timestamps */
    for (int i = 0; i < sizeof(time_t); i++)
        MIX_BYTE(entry->created_at >> (i * 8));

    for (int i = 0; i < sizeof(time_t); i++)
        MIX_BYTE(entry->updated_at >> (i * 8));

    /* Final avalanche */
    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= hash >> 33;

    /* Hex output (64-bit = 16 hex chars) */
    char *out = malloc(17);
    if (!out) return NULL;

    snprintf(out, 17, "%016llX", (unsigned long long)hash);
    return out;
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

/* ============================================================================
 * Utility Functions
 * ============================================================================ */
static bool string_matches_pattern(const char *str, const char *pattern) {
    if (!str || !pattern) return false;

    bool case_insensitive = false;

    /* Optional (?i) prefix */
    if (strncmp(pattern, "(?i)", 4) == 0) {
        case_insensitive = true;
        pattern += 4;
    }

    /* Helpers */
    #define CH_EQ(a, b) \
        (case_insensitive ? (tolower((unsigned char)(a)) == tolower((unsigned char)(b))) : ((a) == (b)))

    /* Prefix match ^foo */
    if (pattern[0] == '^') {
        pattern++;
        while (*pattern && *str) {
            if (!CH_EQ(*str++, *pattern++))
                return false;
        }
        return *pattern == '\0';
    }

    /* Suffix match bar$ */
    size_t plen = strlen(pattern);
    if (plen > 0 && pattern[plen - 1] == '$') {
        pattern = strndup(pattern, plen - 1);
        size_t slen = strlen(str);

        if (slen < plen - 1) return false;

        const char *tail = str + (slen - (plen - 1));
        while (*pattern) {
            if (!CH_EQ(*tail++, *pattern++))
                return false;
        }
        return true;
    }

    /* Wildcard match foo*bar */
    const char *star = strchr(pattern, '*');
    if (star) {
        size_t head_len = star - pattern;
        size_t tail_len = strlen(star + 1);

        /* Head */
        for (size_t i = 0; i < head_len; i++) {
            if (!str[i] || !CH_EQ(str[i], pattern[i]))
                return false;
        }

        /* Tail */
        if (tail_len == 0)
            return true;

        size_t slen = strlen(str);
        if (slen < head_len + tail_len)
            return false;

        const char *tail = str + slen - tail_len;
        const char *pat_tail = star + 1;

        while (*pat_tail) {
            if (!CH_EQ(*tail++, *pat_tail++))
                return false;
        }

        return true;
    }

    /* Default: substring search (case-aware) */
    for (const char *s = str; *s; s++) {
        const char *a = s;
        const char *b = pattern;

        while (*a && *b && CH_EQ(*a, *b)) {
            a++;
            b++;
        }

        if (*b == '\0')
            return true;
    }

    return false;
}

static fossil_bluecrab_core_entry_t *copy_entry(const fossil_bluecrab_core_entry_t *entry) {
    fossil_bluecrab_core_entry_t *e = malloc(sizeof(fossil_bluecrab_core_entry_t));
    e->key = fossil_bluecrab_core_strdup(entry->key);
    e->value = fossil_bluecrab_core_value_copy(&entry->value);
    e->created_at = entry->created_at;
    e->updated_at = entry->updated_at;
    e->metadata = entry->metadata ? fossil_bluecrab_core_strdup(entry->metadata) : NULL;
    e->hash = entry->hash ? strdup(entry->hash) : NULL;
    return e;
}

/* ============================================================================
 * Query / Search Operations (NoSQL-style)
 * ============================================================================ */
size_t fossil_bluecrab_core_find_keys(fossil_bluecrab_core_db_t *db,
                                      const char *pattern,
                                      char ***out_keys) {
    if (!db || !pattern || !out_keys) return 0;

    size_t count = 0;
    for (size_t i = 0; i < db->entry_count; i++)
        if (string_matches_pattern(db->entries[i].key, pattern)) count++;

    *out_keys = malloc(sizeof(char*) * count);
    size_t j = 0;
    for (size_t i = 0; i < db->entry_count; i++) {
        if (string_matches_pattern(db->entries[i].key, pattern))
            (*out_keys)[j++] = fossil_bluecrab_core_strdup(db->entries[i].key);
    }
    return count;
}

size_t fossil_bluecrab_core_find_entries(fossil_bluecrab_core_db_t *db,
                                         const char *pattern,
                                         fossil_bluecrab_core_entry_t **out_entries) {
    if (!db || !pattern || !out_entries) return 0;

    size_t count = 0;
    for (size_t i = 0; i < db->entry_count; i++)
        if (string_matches_pattern(db->entries[i].key, pattern)) count++;

    *out_entries = malloc(sizeof(fossil_bluecrab_core_entry_t) * count);
    size_t j = 0;
    for (size_t i = 0; i < db->entry_count; i++) {
        if (string_matches_pattern(db->entries[i].key, pattern))
            (*out_entries)[j++] = *copy_entry(&db->entries[i]);
    }
    return count;
}

bool fossil_bluecrab_core_clear(fossil_bluecrab_core_db_t *db) {
    if (!db) return false;
    for (size_t i = 0; i < db->entry_count; i++) {
        fossil_bluecrab_core_value_free(&db->entries[i].value);
        free(db->entries[i].key);
        free(db->entries[i].metadata);
        free(db->entries[i].hash);
    }
    free(db->entries);
    db->entries = NULL;
    db->entry_count = 0;
    return true;
}

/* ============================================================================
 * Merge / Diff Operations (Git-style)
 * ============================================================================ */
bool fossil_bluecrab_core_diff(fossil_bluecrab_core_db_t *db,
                               const char *commit_a,
                               const char *commit_b) {
    if (!db || !commit_a || !commit_b) return false;

    fossil_bluecrab_core_commit_t *a = bluecrab_find_commit(db, commit_a);
    fossil_bluecrab_core_commit_t *b = bluecrab_find_commit(db, commit_b);

    if (!a || !b) {
        printf("[BlueCrab][diff] Commit not found\n");
        return false;
    }

    printf("[BlueCrab][diff] %s → %s\n", commit_a, commit_b);

    /* Removed or modified */
    for (size_t i = 0; i < a->snapshot_count; i++) {
        fossil_bluecrab_core_entry_t *ea = &a->snapshot[i];
        fossil_bluecrab_core_entry_t *eb =
            bluecrab_find_entry(b->snapshot, b->snapshot_count, ea->key);

        if (!eb) {
            printf("  - removed: %s\n", ea->key);
        } else if (strcmp(ea->hash, eb->hash) != 0) {
            printf("  ~ modified: %s\n", ea->key);
        }
    }

    /* Added */
    for (size_t i = 0; i < b->snapshot_count; i++) {
        fossil_bluecrab_core_entry_t *eb = &b->snapshot[i];
        fossil_bluecrab_core_entry_t *ea =
            bluecrab_find_entry(a->snapshot, a->snapshot_count, eb->key);

        if (!ea) {
            printf("  + added: %s\n", eb->key);
        }
    }

    return true;
}

bool fossil_bluecrab_core_merge(fossil_bluecrab_core_db_t *db,
                                const char *source_commit,
                                const char *target_commit,
                                bool auto_resolve_conflicts) {
    if (!db || !source_commit || !target_commit) return false;

    fossil_bluecrab_core_commit_t *src =
        bluecrab_find_commit(db, source_commit);
    fossil_bluecrab_core_commit_t *dst =
        bluecrab_find_commit(db, target_commit);

    if (!src || !dst) {
        printf("[BlueCrab][merge] Commit not found\n");
        return false;
    }

    printf("[BlueCrab][merge] %s → %s\n", source_commit, target_commit);

    /* Start from target snapshot */
    fossil_bluecrab_core_clear(db);
    for (size_t i = 0; i < dst->snapshot_count; i++) {
        fossil_bluecrab_core_set(db,
                                 dst->snapshot[i].key,
                                 fossil_bluecrab_core_value_copy(
                                     &dst->snapshot[i].value));
    }

    /* Apply source changes */
    for (size_t i = 0; i < src->snapshot_count; i++) {
        fossil_bluecrab_core_entry_t *se = &src->snapshot[i];
        fossil_bluecrab_core_entry_t *te =
            bluecrab_find_entry(dst->snapshot, dst->snapshot_count, se->key);

        if (!te) {
            /* New key */
            fossil_bluecrab_core_set(db,
                                     se->key,
                                     fossil_bluecrab_core_value_copy(&se->value));
            printf("  + merged: %s\n", se->key);
        } else if (strcmp(se->hash, te->hash) != 0) {
            /* Conflict */
            if (!auto_resolve_conflicts) {
                printf("  ! conflict: %s (merge aborted)\n", se->key);
                return false;
            }

            fossil_bluecrab_core_set(db,
                                     se->key,
                                     fossil_bluecrab_core_value_copy(&se->value));
            printf("  ~ conflict resolved (source): %s\n", se->key);
        }
    }

    fossil_bluecrab_core_commit(db, "merge commit");
    return true;
}

/* ============================================================================
 * Tagging / Bookmarking Commits
 * ============================================================================ */
typedef struct fossil_bluecrab_core_tag_s {
    char *name;
    char *commit_hash;
    struct fossil_bluecrab_core_tag_s *next;
} fossil_bluecrab_core_tag_t;

static fossil_bluecrab_core_tag_t *tag_head = NULL;

bool fossil_bluecrab_core_tag_commit(fossil_bluecrab_core_db_t *db,
                                     const char *commit_hash,
                                     const char *tag_name) {
    if (!db || !commit_hash || !tag_name) return false;

    fossil_bluecrab_core_tag_t *t = malloc(sizeof(fossil_bluecrab_core_tag_t));
    t->name = fossil_bluecrab_core_strdup(tag_name);
    t->commit_hash = fossil_bluecrab_core_strdup(commit_hash);
    t->next = tag_head;
    tag_head = t;

    printf("[BlueCrab] Tagged commit %s as %s\n", commit_hash, tag_name);
    return true;
}

char *fossil_bluecrab_core_get_tagged_commit(fossil_bluecrab_core_db_t *db,
                                             const char *tag_name) {
    fossil_bluecrab_core_tag_t *t = tag_head;
    while (t) {
        if (strcmp(t->name, tag_name) == 0) return t->commit_hash;
        t = t->next;
    }
    return NULL;
}

/* ============================================================================
 * Entry Metadata / Extended Utilities
 * ============================================================================ */
bool fossil_bluecrab_core_set_metadata(fossil_bluecrab_core_db_t *db,
                                       const char *key,
                                       const char *metadata) {
    if (!db || !key) return false;
    for (size_t i = 0; i < db->entry_count; i++) {
        if (strcmp(db->entries[i].key, key) == 0) {
            free(db->entries[i].metadata);
            db->entries[i].metadata = metadata ? strdup(metadata) : NULL;
            return true;
        }
    }
    return false;
}

char *fossil_bluecrab_core_get_metadata(fossil_bluecrab_core_db_t *db,
                                        const char *key) {
    if (!db || !key) return NULL;
    for (size_t i = 0; i < db->entry_count; i++)
        if (strcmp(db->entries[i].key, key) == 0) return db->entries[i].metadata;
    return NULL;
}

bool fossil_bluecrab_core_has_key(fossil_bluecrab_core_db_t *db,
                                  const char *key) {
    if (!db || !key) return false;
    for (size_t i = 0; i < db->entry_count; i++)
        if (strcmp(db->entries[i].key, key) == 0) return true;
    return false;
}

/* ============================================================================
 * Advanced Hash / Tamper Detection
 * ============================================================================ */
bool fossil_bluecrab_core_verify_entry(fossil_bluecrab_core_entry_t *entry) {
    if (!entry) return false;
    if (!entry->hash) return false;
    char *computed = fossil_bluecrab_core_hash_entry(entry);
    bool ok = computed && strcmp(computed, entry->hash) == 0;
    free(computed);
    return ok;
}

bool fossil_bluecrab_core_verify_db(fossil_bluecrab_core_db_t *db) {
    if (!db) return false;
    for (size_t i = 0; i < db->entry_count; i++)
        if (!fossil_bluecrab_core_verify_entry(&db->entries[i])) return false;
    return true;
}

/* ============================================================================
 * Utility / Debug
 * ============================================================================ */
void fossil_bluecrab_core_print_entry(fossil_bluecrab_core_entry_t *entry) {
    if (!entry) return;
    printf("Key: %s, Type: %s, Created: %ld, Updated: %ld\n",
           entry->key,
           fossil_bluecrab_core_type_to_string(entry->value.type),
           entry->created_at,
           entry->updated_at);
    if (entry->metadata) printf("Metadata: %s\n", entry->metadata);
}

void fossil_bluecrab_core_print_db(fossil_bluecrab_core_db_t *db) {
    if (!db) return;
    printf("Database Path: %s\n", db->db_path);
    printf("Entries: %zu\n", db->entry_count);
    for (size_t i = 0; i < db->entry_count; i++) {
        fossil_bluecrab_core_print_entry(&db->entries[i]);
    }
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
