/*
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
#ifndef FOSSIL_BLUECRAB_STORE_H
#define FOSSIL_BLUECRAB_STORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Handle
// -----------------------------------------------------------------------------
typedef struct fossil_store fossil_store_t;

// -----------------------------------------------------------------------------
// Open/Close
// -----------------------------------------------------------------------------
fossil_store_t* fossil_bluecrab_store_open(const char* path, bool create_if_missing);
void fossil_bluecrab_store_close(fossil_store_t* store);

// -----------------------------------------------------------------------------
// Data Operations
// -----------------------------------------------------------------------------
bool fossil_bluecrab_store_put(fossil_store_t* store,
                               const char* key,
                               const char* value_json);

char* fossil_bluecrab_store_get(fossil_store_t* store,
                                const char* key);

bool fossil_bluecrab_store_remove(fossil_store_t* store,
                                  const char* key);

size_t fossil_bluecrab_store_count(fossil_store_t* store);

// -----------------------------------------------------------------------------
// Integrity
// -----------------------------------------------------------------------------
uint64_t fossil_bluecrab_store_hash(const char* data,
                                    size_t len,
                                    uint64_t salt,
                                    uint64_t timestamp);

// -----------------------------------------------------------------------------
// Iteration
// -----------------------------------------------------------------------------
typedef void (*fossil_store_iter_cb)(const char* key,
                                     const char* value_json,
                                     void* userdata);

void fossil_bluecrab_store_iterate(fossil_store_t* store,
                                   fossil_store_iter_cb cb,
                                   void* userdata);

#ifdef __cplusplus
}
#endif

#endif // FOSSIL_BLUECRAB_STORE_H
