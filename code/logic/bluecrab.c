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
#include <stdint.h>
#include <time.h>

/* ============================================================================
 * Internal Hash Algorithm (Blake3-inspired lightweight)
 * ========================================================================== */
static void fossil_bluecrab_core_hash(
    const void *data, size_t size, fossil_bluecrab_core_hash_t *out_hash)
{
    const uint8_t *ptr = (const uint8_t *)data;
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    for (size_t i = 0; i < size; ++i) {
        uint32_t x = ptr[i];
        h[i % 8] ^= x;
        h[(i+1) % 8] += ((h[(i+2) % 8] << 5) | (h[(i+2) % 8] >> 27)) ^ x;
        h[(i+3) % 8] ^= ((h[(i+4) % 8] >> 3) | (h[(i+4) % 8] << 29));
    }

    for (int i = 0; i < 8; ++i) {
        out_hash->bytes[i*4 + 0] = (h[i] >> 24) & 0xFF;
        out_hash->bytes[i*4 + 1] = (h[i] >> 16) & 0xFF;
        out_hash->bytes[i*4 + 2] = (h[i] >> 8) & 0xFF;
        out_hash->bytes[i*4 + 3] = (h[i] >> 0) & 0xFF;
    }
}

/* ============================================================================
 * Database Structure (persistent)
 * ========================================================================== */
struct fossil_bluecrab_core_db {
    fossil_bluecrab_core_storage_ops_t storage;
    uint64_t last_record_id;
    fossil_bluecrab_core_hash_t last_hash;
};

/* ============================================================================
 * Internal Helpers
 * ========================================================================== */
static fossil_bluecrab_core_result_t fossil_bluecrab_core_write_record(
    fossil_bluecrab_core_db_t *db,
    const fossil_bluecrab_core_record_t *record)
{
    uint64_t offset = (record->record_id - 1) * sizeof(fossil_bluecrab_core_record_t);
    return db->storage.write(db->storage.ctx, offset, record, sizeof(fossil_bluecrab_core_record_t));
}

static fossil_bluecrab_core_result_t fossil_bluecrab_core_read_record(
    fossil_bluecrab_core_db_t *db,
    uint64_t record_id,
    fossil_bluecrab_core_record_t *out_record)
{
    uint64_t offset = (record_id - 1) * sizeof(fossil_bluecrab_core_record_t);
    return db->storage.read(db->storage.ctx, offset, out_record, sizeof(fossil_bluecrab_core_record_t));
}

/* ============================================================================
 * Core Lifecycle
 * ========================================================================== */
fossil_bluecrab_core_result_t
fossil_bluecrab_core_init(
    fossil_bluecrab_core_db_t **out_db,
    const fossil_bluecrab_core_storage_ops_t *storage_ops)
{
    if (!out_db || !storage_ops) return FOSSIL_BLUECRAB_ERR_INVALID_ARG;

    fossil_bluecrab_core_db_t *db = malloc(sizeof(fossil_bluecrab_core_db_t));
    if (!db) return FOSSIL_BLUECRAB_ERR_GENERIC;

    db->storage = *storage_ops;
    db->last_record_id = 0;
    memset(&db->last_hash, 0, sizeof(fossil_bluecrab_core_hash_t));

    *out_db = db;
    return FOSSIL_BLUECRAB_OK;
}

void fossil_bluecrab_core_shutdown(fossil_bluecrab_core_db_t *db)
{
    if (db) free(db);
}

/* ============================================================================
 * Insert & Fetch Records
 * ========================================================================== */
fossil_bluecrab_core_result_t
fossil_bluecrab_core_insert(
    fossil_bluecrab_core_db_t *db,
    const fossil_bluecrab_core_value_t *value,
    uint64_t *out_record_id)
{
    if (!db || !value || !out_record_id) return FOSSIL_BLUECRAB_ERR_INVALID_ARG;

    fossil_bluecrab_core_record_t record = {0};
    record.record_id = ++db->last_record_id;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    record.timestamp.wall_time_ns = (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
    record.timestamp.monotonic_ns = record.timestamp.wall_time_ns;

    record.value = *value;
    record.confidence_score = 1.0f;
    record.usage_count = 0;

    /* Previous hash for tamper chain */
    record.prev_hash = db->last_hash;

    /* Compute current hash */
    fossil_bluecrab_core_hash(&record, sizeof(fossil_bluecrab_core_record_t), &record.self_hash);

    /* Update DB state */
    db->last_hash = record.self_hash;

    /* Persist */
    fossil_bluecrab_core_result_t res = fossil_bluecrab_core_write_record(db, &record);
    if (res != FOSSIL_BLUECRAB_OK) return res;

    *out_record_id = record.record_id;
    return FOSSIL_BLUECRAB_OK;
}

fossil_bluecrab_core_result_t
fossil_bluecrab_core_fetch(
    fossil_bluecrab_core_db_t *db,
    uint64_t record_id,
    fossil_bluecrab_core_record_t *out_record)
{
    if (!db || !out_record) return FOSSIL_BLUECRAB_ERR_INVALID_ARG;
    if (record_id == 0 || record_id > db->last_record_id) return FOSSIL_BLUECRAB_ERR_NOT_FOUND;

    return fossil_bluecrab_core_read_record(db, record_id, out_record);
}

/* ============================================================================
 * Verify Tamper Chain
 * ========================================================================== */
fossil_bluecrab_core_result_t
fossil_bluecrab_core_verify_chain(fossil_bluecrab_core_db_t *db)
{
    if (!db) return FOSSIL_BLUECRAB_ERR_INVALID_ARG;

    fossil_bluecrab_core_hash_t prev_hash;
    memset(&prev_hash, 0, sizeof(prev_hash));

    fossil_bluecrab_core_record_t record;
    for (uint64_t i = 1; i <= db->last_record_id; ++i) {
        if (fossil_bluecrab_core_read_record(db, i, &record) != FOSSIL_BLUECRAB_OK)
            return FOSSIL_BLUECRAB_ERR_IO;

        if (memcmp(&record.prev_hash, &prev_hash, sizeof(prev_hash)) != 0)
            return FOSSIL_BLUECRAB_ERR_TAMPERED;

        fossil_bluecrab_core_hash(&record, sizeof(record), &prev_hash);
    }

    return FOSSIL_BLUECRAB_OK;
}

/* ============================================================================
 * Rehash All Records (Useful for Recovery)
 * ========================================================================== */
fossil_bluecrab_core_result_t
fossil_bluecrab_core_rehash_all(fossil_bluecrab_core_db_t *db)
{
    if (!db) return FOSSIL_BLUECRAB_ERR_INVALID_ARG;

    fossil_bluecrab_core_hash_t prev_hash;
    memset(&prev_hash, 0, sizeof(prev_hash));

    fossil_bluecrab_core_record_t record;
    for (uint64_t i = 1; i <= db->last_record_id; ++i) {
        if (fossil_bluecrab_core_read_record(db, i, &record) != FOSSIL_BLUECRAB_OK)
            return FOSSIL_BLUECRAB_ERR_IO;

        record.prev_hash = prev_hash;
        fossil_bluecrab_core_hash(&record, sizeof(record), &record.self_hash);
        if (fossil_bluecrab_core_write_record(db, &record) != FOSSIL_BLUECRAB_OK)
            return FOSSIL_BLUECRAB_ERR_IO;

        prev_hash = record.self_hash;
    }

    db->last_hash = prev_hash;
    return FOSSIL_BLUECRAB_OK;
}

/* ============================================================================
 * Git-Style Commit (Snapshot)
 * ========================================================================== */
fossil_bluecrab_core_result_t
fossil_bluecrab_core_commit(
    fossil_bluecrab_core_db_t *db,
    fossil_bluecrab_core_commit_t *out_commit)
{
    if (!db || !out_commit) return FOSSIL_BLUECRAB_ERR_INVALID_ARG;

    out_commit->parent_hash = db->last_hash;
    out_commit->record_count = db->last_record_id;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    out_commit->timestamp.wall_time_ns = (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
    out_commit->timestamp.monotonic_ns = out_commit->timestamp.wall_time_ns;

    /* Commit hash = hash of all records + timestamp */
    fossil_bluecrab_core_hash(&out_commit->record_count, sizeof(uint64_t), &out_commit->commit_hash);
    db->last_hash = out_commit->commit_hash;

    return FOSSIL_BLUECRAB_OK;
}

/* ============================================================================
 * Commit Index (simple in-memory table for demo purposes)
 * ========================================================================== */

#define FOSSIL_BLUECRAB_MAX_COMMITS 1024

typedef struct fossil_bluecrab_core_commit_index {
    fossil_bluecrab_core_commit_t commits[FOSSIL_BLUECRAB_MAX_COMMITS];
    size_t count;
} fossil_bluecrab_core_commit_index_t;

/* Attach commit index to DB (simple in-memory for now) */
struct fossil_bluecrab_core_db {
    fossil_bluecrab_core_storage_ops_t storage;
    uint64_t last_record_id;
    fossil_bluecrab_core_hash_t last_hash;

    fossil_bluecrab_core_commit_index_t commit_index;
};

/* ============================================================================
 * Checkout by commit hash
 * ========================================================================== */
fossil_bluecrab_core_result_t
fossil_bluecrab_core_checkout(
    fossil_bluecrab_core_db_t *db,
    const fossil_bluecrab_core_hash_t *commit_hash)
{
    if (!db || !commit_hash) return FOSSIL_BLUECRAB_ERR_INVALID_ARG;

    for (size_t i = 0; i < db->commit_index.count; ++i) {
        if (memcmp(&db->commit_index.commits[i].commit_hash, commit_hash,
                   sizeof(fossil_bluecrab_core_hash_t)) == 0)
        {
            /* Restore state to commit */
            db->last_record_id = db->commit_index.commits[i].record_count;
            db->last_hash = db->commit_index.commits[i].commit_hash;
            return FOSSIL_BLUECRAB_OK;
        }
    }

    return FOSSIL_BLUECRAB_ERR_NOT_FOUND;
}

/* ============================================================================
 * Diff between two commits
 * ========================================================================== */
fossil_bluecrab_core_result_t
fossil_bluecrab_core_diff(
    fossil_bluecrab_core_db_t *db,
    const fossil_bluecrab_core_hash_t *a,
    const fossil_bluecrab_core_hash_t *b)
{
    if (!db || !a || !b) return FOSSIL_BLUECRAB_ERR_INVALID_ARG;

    fossil_bluecrab_core_commit_t *commit_a = NULL;
    fossil_bluecrab_core_commit_t *commit_b = NULL;

    for (size_t i = 0; i < db->commit_index.count; ++i) {
        if (memcmp(&db->commit_index.commits[i].commit_hash, a, sizeof(fossil_bluecrab_core_hash_t)) == 0)
            commit_a = &db->commit_index.commits[i];
        if (memcmp(&db->commit_index.commits[i].commit_hash, b, sizeof(fossil_bluecrab_core_hash_t)) == 0)
            commit_b = &db->commit_index.commits[i];
    }

    if (!commit_a || !commit_b) return FOSSIL_BLUECRAB_ERR_NOT_FOUND;

    uint64_t max_records = commit_a->record_count > commit_b->record_count ? 
                           commit_a->record_count : commit_b->record_count;

    fossil_bluecrab_core_record_t rec_a, rec_b;
    for (uint64_t i = 1; i <= max_records; ++i) {
        fossil_bluecrab_core_result_t res_a = fossil_bluecrab_core_read_record(db, i, &rec_a);
        fossil_bluecrab_core_result_t res_b = fossil_bluecrab_core_read_record(db, i, &rec_b);

        /* Only consider records that exist in either commit */
        if (i > commit_a->record_count) { 
            printf("Record %llu added in commit B\n", i); 
            continue; 
        }
        if (i > commit_b->record_count) { 
            printf("Record %llu removed in commit B\n", i); 
            continue; 
        }

        /* Compare hashes to detect modifications */
        if (memcmp(&rec_a.self_hash, &rec_b.self_hash, sizeof(fossil_bluecrab_core_hash_t)) != 0) {
            printf("Record %llu modified\n", i);
        }
    }

    return FOSSIL_BLUECRAB_OK;
}

/* ============================================================================
 * Updated commit function to store in commit index
 * ========================================================================== */
fossil_bluecrab_core_result_t
fossil_bluecrab_core_commit(
    fossil_bluecrab_core_db_t *db,
    fossil_bluecrab_core_commit_t *out_commit)
{
    if (!db || !out_commit) return FOSSIL_BLUECRAB_ERR_INVALID_ARG;

    out_commit->parent_hash = db->last_hash;
    out_commit->record_count = db->last_record_id;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    out_commit->timestamp.wall_time_ns = (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
    out_commit->timestamp.monotonic_ns = out_commit->timestamp.wall_time_ns;

    /* Commit hash = hash of last record hash + record count */
    fossil_bluecrab_core_hash(&db->last_hash, sizeof(fossil_bluecrab_core_hash_t), &out_commit->commit_hash);

    db->last_hash = out_commit->commit_hash;

    /* Store in commit index */
    if (db->commit_index.count < FOSSIL_BLUECRAB_MAX_COMMITS) {
        db->commit_index.commits[db->commit_index.count++] = *out_commit;
    } else {
        return FOSSIL_BLUECRAB_ERR_GENERIC;
    }

    return FOSSIL_BLUECRAB_OK;
}

/* ============================================================================
 * AI-Oriented Introspection
 * ========================================================================== */
fossil_bluecrab_core_result_t
fossil_bluecrab_core_score_record(
    fossil_bluecrab_core_db_t *db,
    uint64_t record_id,
    float delta)
{
    fossil_bluecrab_core_record_t record;
    if (!db || record_id == 0 || record_id > db->last_record_id) return FOSSIL_BLUECRAB_ERR_INVALID_ARG;

    if (fossil_bluecrab_core_read_record(db, record_id, &record) != FOSSIL_BLUECRAB_OK)
        return FOSSIL_BLUECRAB_ERR_IO;

    record.confidence_score += delta;
    if (record.confidence_score < 0) record.confidence_score = 0;

    return fossil_bluecrab_core_write_record(db, &record);
}

fossil_bluecrab_core_result_t
fossil_bluecrab_core_touch_record(
    fossil_bluecrab_core_db_t *db,
    uint64_t record_id)
{
    fossil_bluecrab_core_record_t record;
    if (!db || record_id == 0 || record_id > db->last_record_id) return FOSSIL_BLUECRAB_ERR_INVALID_ARG;

    if (fossil_bluecrab_core_read_record(db, record_id, &record) != FOSSIL_BLUECRAB_OK)
        return FOSSIL_BLUECRAB_ERR_IO;

    record.usage_count += 1;
    return fossil_bluecrab_core_write_record(db, &record);
}
