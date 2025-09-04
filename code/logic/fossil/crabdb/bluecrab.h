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
#ifndef FOSSIL_BLUECRAB_H
#define FOSSIL_BLUECRAB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Tag for union */
typedef enum {
    FBC_T_I8,
    FBC_T_I16,
    FBC_T_I32,
    FBC_T_I64,
    FBC_T_U8,
    FBC_T_U16,
    FBC_T_U32,
    FBC_T_U64,
    FBC_T_HEX,
    FBC_T_OCT,
    FBC_T_F32,
    FBC_T_F64,
    FBC_T_CSTR,
    FBC_T_CHAR,
    FBC_T_UNKNOWN
} fossil_bluecrab_type_t;

/* Union to store any type */
typedef union {
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
    char    *cstr; /* allocated on parse for CSTR */
    char     ch;
} fossil_bluecrab_value_t;

/* Parse a string into a fossil_bluecrab_value_t according to type tag.
 * Returns true on success; false on parse error.
 * For FBC_T_CSTR the function will allocate memory (strdup-like). Caller
 * must call fossil_bluecrab_free_value() to free cstr.
 */
bool fossil_bluecrab_parse(fossil_bluecrab_type_t tag, const char *str, fossil_bluecrab_value_t *out);

/* Format the value into a newly malloc'd string (caller frees). For CSTR
 * this returns a strdup of the string. Returns NULL on error (or OOM).
 */
char *fossil_bluecrab_to_string(fossil_bluecrab_type_t tag, const fossil_bluecrab_value_t *val);

/* Map type name (like "i8", "u32", "hex", "cstr") to tag. Case-insensitive.
 * Returns FBC_T_UNKNOWN on failure.
 */
fossil_bluecrab_type_t fossil_bluecrab_type_from_string(const char *name);

/* Map tag to canonical string name ("i8", "u32", ...). Returns "unknown" for unknown.
 * The returned pointer is static; do not free.
 */
const char *fossil_bluecrab_type_to_string(fossil_bluecrab_type_t tag);

/* Free value resources if needed (only relevant for cstr). Safe to call on any tag/value. */
void fossil_bluecrab_free_value(fossil_bluecrab_type_t tag, fossil_bluecrab_value_t *val);

/* Compare two values of same tag for equality (for numeric types compares numeric equality,
 * for cstr compares strcmp, for floats uses exact bitwise compare via memcmp). Returns true if equal.
 */
bool fossil_bluecrab_equal(fossil_bluecrab_type_t tag, const fossil_bluecrab_value_t *a, const fossil_bluecrab_value_t *b);

/* 64-bit hash function that accepts only str as parameter but uses an internal runtime
 * salt derived from time + pointer entropy per-process. Deterministic within a process lifetime.
 */
static uint64_t fossil_bluecrab_hash(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* FOSSIL_BLUECRAB_H */
