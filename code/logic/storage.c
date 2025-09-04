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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* --- internal entry --- */
typedef struct {
    char *key;
    fossil_bluecrab_type_t tag;
    fossil_bluecrab_value_t val;
    bool in_use;
} fbc_entry_t;

struct fossil_bluecrab_storeage {
    fbc_entry_t *entries;
    size_t capacity;
    size_t count;
};

/* --- helpers --- */
static size_t fbc_mod_hash(const char *key, size_t cap) {
    uint64_t h = fossil_bluecrab_hash(key);
    return (size_t)(h % cap);
}

/* --- API --- */
fossil_bluecrab_storeage_t *fossil_bluecrab_storeage_create(size_t initial_capacity) {
    fossil_bluecrab_storeage_t *st = (fossil_bluecrab_storeage_t *)calloc(1, sizeof(*st));
    if (!st) return NULL;
    if (initial_capacity < 8) initial_capacity = 8;
    st->entries = (fbc_entry_t *)calloc(initial_capacity, sizeof(fbc_entry_t));
    if (!st->entries) {
        free(st);
        return NULL;
    }
    st->capacity = initial_capacity;
    st->count = 0;
    return st;
}

void fossil_bluecrab_storeage_destroy(fossil_bluecrab_storeage_t *st) {
    if (!st) return;
    for (size_t i = 0; i < st->capacity; i++) {
        if (st->entries[i].in_use) {
            free(st->entries[i].key);
            fossil_bluecrab_free_value(st->entries[i].tag, &st->entries[i].val);
        }
    }
    free(st->entries);
    free(st);
}

static bool fbc_store_rehash(fossil_bluecrab_storeage_t *st, size_t newcap) {
    fbc_entry_t *old = st->entries;
    size_t oldcap = st->capacity;

    fbc_entry_t *new_entries = (fbc_entry_t *)calloc(newcap, sizeof(fbc_entry_t));
    if (!new_entries) return false;

    st->entries = new_entries;
    st->capacity = newcap;
    st->count = 0;

    for (size_t i = 0; i < oldcap; i++) {
        if (old[i].in_use) {
            fossil_bluecrab_storeage_set(st, old[i].key, old[i].tag, &old[i].val);
            free(old[i].key);
            fossil_bluecrab_free_value(old[i].tag, &old[i].val);
        }
    }
    free(old);
    return true;
}

bool fossil_bluecrab_storeage_set(fossil_bluecrab_storeage_t *st,
                                  const char *key,
                                  fossil_bluecrab_type_t tag,
                                  const fossil_bluecrab_value_t *val) {
    if (!st || !key || !val) return false;
    if (st->count * 2 >= st->capacity) {
        if (!fbc_store_rehash(st, st->capacity * 2)) return false;
    }
    size_t idx = fbc_mod_hash(key, st->capacity);
    for (size_t i = 0; i < st->capacity; i++) {
        size_t probe = (idx + i) % st->capacity;
        fbc_entry_t *e = &st->entries[probe];
        if (!e->in_use) {
            e->key = strdup(key);
            e->tag = tag;
            memset(&e->val, 0, sizeof(e->val));
            fossil_bluecrab_parse(tag, fossil_bluecrab_to_string(tag, val), &e->val);
            e->in_use = true;
            st->count++;
            return true;
        }
        if (strcmp(e->key, key) == 0) {
            fossil_bluecrab_free_value(e->tag, &e->val);
            e->tag = tag;
            fossil_bluecrab_parse(tag, fossil_bluecrab_to_string(tag, val), &e->val);
            return true;
        }
    }
    return false;
}

bool fossil_bluecrab_storeage_get(fossil_bluecrab_storeage_t *st,
                                  const char *key,
                                  fossil_bluecrab_type_t *tag_out,
                                  fossil_bluecrab_value_t *val_out) {
    if (!st || !key) return false;
    size_t idx = fbc_mod_hash(key, st->capacity);
    for (size_t i = 0; i < st->capacity; i++) {
        size_t probe = (idx + i) % st->capacity;
        fbc_entry_t *e = &st->entries[probe];
        if (!e->in_use) continue;
        if (strcmp(e->key, key) == 0) {
            if (tag_out) *tag_out = e->tag;
            if (val_out) {
                char *s = fossil_bluecrab_to_string(e->tag, &e->val);
                fossil_bluecrab_parse(e->tag, s, val_out);
                free(s);
            }
            return true;
        }
    }
    return false;
}

bool fossil_bluecrab_storeage_remove(fossil_bluecrab_storeage_t *st, const char *key) {
    if (!st || !key) return false;
    size_t idx = fbc_mod_hash(key, st->capacity);
    for (size_t i = 0; i < st->capacity; i++) {
        size_t probe = (idx + i) % st->capacity;
        fbc_entry_t *e = &st->entries[probe];
        if (!e->in_use) continue;
        if (strcmp(e->key, key) == 0) {
            free(e->key);
            fossil_bluecrab_free_value(e->tag, &e->val);
            memset(e, 0, sizeof(*e));
            st->count--;
            return true;
        }
    }
    return false;
}

bool fossil_bluecrab_storeage_save(fossil_bluecrab_storeage_t *st, const char *filename) {
    if (!st || !filename) return false;
    FILE *f = fopen(filename, "w");
    if (!f) return false;
    for (size_t i = 0; i < st->capacity; i++) {
        if (st->entries[i].in_use) {
            char *valstr = fossil_bluecrab_to_string(st->entries[i].tag, &st->entries[i].val);
            fprintf(f, "%s %s %s\n",
                    st->entries[i].key,
                    fossil_bluecrab_type_to_string(st->entries[i].tag),
                    valstr ? valstr : "");
            free(valstr);
        }
    }
    fclose(f);
    return true;
}

bool fossil_bluecrab_storeage_load(fossil_bluecrab_storeage_t *st, const char *filename) {
    if (!st || !filename) return false;
    FILE *f = fopen(filename, "r");
    if (!f) return false;
    char key[256], type[32], val[512];
    while (fscanf(f, "%255s %31s %511s", key, type, val) == 3) {
        fossil_bluecrab_type_t tag = fossil_bluecrab_type_from_string(type);
        fossil_bluecrab_value_t v;
        if (fossil_bluecrab_parse(tag, val, &v)) {
            fossil_bluecrab_storeage_set(st, key, tag, &v);
            fossil_bluecrab_free_value(tag, &v);
        }
    }
    fclose(f);
    return true;
}

size_t fossil_bluecrab_storeage_count(fossil_bluecrab_storeage_t *st) {
    return st ? st->count : 0;
}
