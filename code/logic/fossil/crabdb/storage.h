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

#include "bluecrab.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fossil_bluecrab_storeage fossil_bluecrab_storeage_t;

/* Create/destroy a store */
fossil_bluecrab_storeage_t *fossil_bluecrab_storeage_create(size_t initial_capacity);
void fossil_bluecrab_storeage_destroy(fossil_bluecrab_storeage_t *st);

/* Insert or update a key-value */
bool fossil_bluecrab_storeage_set(fossil_bluecrab_storeage_t *st,
                                  const char *key,
                                  fossil_bluecrab_type_t tag,
                                  const fossil_bluecrab_value_t *val);

/* Get a value by key; returns false if not found */
bool fossil_bluecrab_storeage_get(fossil_bluecrab_storeage_t *st,
                                  const char *key,
                                  fossil_bluecrab_type_t *tag_out,
                                  fossil_bluecrab_value_t *val_out);

/* Remove a key */
bool fossil_bluecrab_storeage_remove(fossil_bluecrab_storeage_t *st, const char *key);

/* Save/load to file (very simple text format) */
bool fossil_bluecrab_storeage_save(fossil_bluecrab_storeage_t *st, const char *filename);
bool fossil_bluecrab_storeage_load(fossil_bluecrab_storeage_t *st, const char *filename);

/* Count of items */
size_t fossil_bluecrab_storeage_count(fossil_bluecrab_storeage_t *st);

#ifdef __cplusplus
}
#endif

#endif // FOSSIL_BLUECRAB_STORE_H
