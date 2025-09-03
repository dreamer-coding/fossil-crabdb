/**
 * -----------------------------------------------------------------------------
 * Project: Fossil Logic
 *
 * This file is part of the Fossil Logic project, which aims to develop high-
 * performance, cross-platform applications and libraries. The code contained
 * herein is subject to the terms and conditions defined in the project license.
 *
 * Author: Michael Gene Brockus (Dreamer)
 *
 * Copyright (C) 2024 Fossil Logic. All rights reserved.
 * -----------------------------------------------------------------------------
 */
#include "fossil/crabdb/storage.h"
#include <time.h>

// -----------------------------------------------------------------------------
// Internal record format
// -----------------------------------------------------------------------------
typedef struct {
    uint32_t key_len;
    uint32_t value_len;
    uint64_t timestamp;
    uint64_t hash;
    // followed by key bytes, then value bytes
} record_header_t;

struct fossil_store {
    FILE* fp;
    char* path;
    size_t count;
};

// -----------------------------------------------------------------------------
// Simple 64-bit FNV-1a hash (salt + timestamp mixed in)
// -----------------------------------------------------------------------------
uint64_t fossil_bluecrab_store_hash(const char* data,
                                    size_t len,
                                    uint64_t salt,
                                    uint64_t timestamp) {
    const uint64_t FNV_OFFSET = 1469598103934665603ULL;
    const uint64_t FNV_PRIME  = 1099511628211ULL;

    uint64_t h = FNV_OFFSET ^ salt ^ timestamp;
    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned char)data[i];
        h *= FNV_PRIME;
    }
    return h;
}

// -----------------------------------------------------------------------------
// Open/Close
// -----------------------------------------------------------------------------
fossil_store_t* fossil_bluecrab_store_open(const char* path, bool create_if_missing) {
    if (!path) return NULL;

    FILE* fp = fopen(path, "r+b");
    if (!fp && create_if_missing) {
        fp = fopen(path, "w+b");
    }
    if (!fp) return NULL;

    fossil_store_t* store = calloc(1, sizeof(*store));
    store->fp = fp;
    store->path = strdup(path);

    // Count records
    rewind(fp);
    size_t count = 0;
    while (1) {
        record_header_t hdr;
        if (fread(&hdr, sizeof(hdr), 1, fp) != 1) break;
        fseek(fp, hdr.key_len + hdr.value_len, SEEK_CUR);
        count++;
    }
    store->count = count;
    rewind(fp);

    return store;
}

void fossil_bluecrab_store_close(fossil_store_t* store) {
    if (!store) return;
    if (store->fp) fclose(store->fp);
    free(store->path);
    free(store);
}

// -----------------------------------------------------------------------------
// Put (append new record)
// -----------------------------------------------------------------------------
bool fossil_bluecrab_store_put(fossil_store_t* store,
                               const char* key,
                               const char* value_json) {
    if (!store || !key || !value_json) return false;

    uint32_t klen = (uint32_t)strlen(key);
    uint32_t vlen = (uint32_t)strlen(value_json);
    uint64_t ts   = (uint64_t)time(NULL);

    uint64_t h = fossil_bluecrab_store_hash(key, klen, 0xABCDEF1234567890ULL, ts);
    h ^= fossil_bluecrab_store_hash(value_json, vlen, 0x1111111111111111ULL, ts);

    record_header_t hdr = { klen, vlen, ts, h };

    fseek(store->fp, 0, SEEK_END);
    if (fwrite(&hdr, sizeof(hdr), 1, store->fp) != 1) return false;
    if (fwrite(key, 1, klen, store->fp) != klen) return false;
    if (fwrite(value_json, 1, vlen, store->fp) != vlen) return false;
    fflush(store->fp);

    store->count++;
    return true;
}

// -----------------------------------------------------------------------------
// Get (scan for last record with given key)
// -----------------------------------------------------------------------------
char* fossil_bluecrab_store_get(fossil_store_t* store,
                                const char* key) {
    if (!store || !key) return NULL;

    rewind(store->fp);
    record_header_t hdr;
    char* result = NULL;

    while (fread(&hdr, sizeof(hdr), 1, store->fp) == 1) {
        char* kbuf = malloc(hdr.key_len + 1);
        char* vbuf = malloc(hdr.value_len + 1);
        fread(kbuf, 1, hdr.key_len, store->fp);
        fread(vbuf, 1, hdr.value_len, store->fp);
        kbuf[hdr.key_len] = '\0';
        vbuf[hdr.value_len] = '\0';

        if (strcmp(kbuf, key) == 0) {
            free(result);
            result = strdup(vbuf);
        }
        free(kbuf);
        free(vbuf);
    }

    return result;
}

// -----------------------------------------------------------------------------
// Remove (append a tombstone record)
// -----------------------------------------------------------------------------
bool fossil_bluecrab_store_remove(fossil_store_t* store,
                                  const char* key) {
    if (!store || !key) return false;

    uint32_t klen = (uint32_t)strlen(key);
    uint32_t vlen = 0;
    uint64_t ts   = (uint64_t)time(NULL);
    uint64_t h    = fossil_bluecrab_store_hash(key, klen, 0xDEADBEAFDEADBEAFULL, ts);

    record_header_t hdr = { klen, vlen, ts, h };

    fseek(store->fp, 0, SEEK_END);
    if (fwrite(&hdr, sizeof(hdr), 1, store->fp) != 1) return false;
    if (fwrite(key, 1, klen, store->fp) != klen) return false;
    fflush(store->fp);

    store->count++;
    return true;
}

// -----------------------------------------------------------------------------
// Count (cheap: stored in memory)
// -----------------------------------------------------------------------------
size_t fossil_bluecrab_store_count(fossil_store_t* store) {
    if (!store) return 0;
    return store->count;
}

// -----------------------------------------------------------------------------
// Iterate over all records
// -----------------------------------------------------------------------------
void fossil_bluecrab_store_iterate(fossil_store_t* store,
                                   fossil_store_iter_cb cb,
                                   void* userdata) {
    if (!store || !cb) return;

    rewind(store->fp);
    record_header_t hdr;

    while (fread(&hdr, sizeof(hdr), 1, store->fp) == 1) {
        char* kbuf = malloc(hdr.key_len + 1);
        char* vbuf = NULL;

        fread(kbuf, 1, hdr.key_len, store->fp);
        kbuf[hdr.key_len] = '\0';

        if (hdr.value_len > 0) {
            vbuf = malloc(hdr.value_len + 1);
            fread(vbuf, 1, hdr.value_len, store->fp);
            vbuf[hdr.value_len] = '\0';
        }

        cb(kbuf, vbuf, userdata);

        free(kbuf);
        free(vbuf);
    }
}
