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
#include "fossil/crabdb/bluecrab.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <unistd.h> /* for getpid on POSIX; if not available, salt still works */


/* ---------- helper: case-insensitive compare ---------- */
static int fbc_stricmp(const char *a, const char *b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return (ca - cb);
        a++; b++;
    }
    return (int)(unsigned char)tolower((unsigned char)*a) - (int)(unsigned char)tolower((unsigned char)*b);
}

/* ---------- type mapping ---------- */
fossil_bluecrab_type_t fossil_bluecrab_type_from_string(const char *name) {
    if (!name) return FBC_T_UNKNOWN;
    if (fbc_stricmp(name, "i8") == 0) return FBC_T_I8;
    if (fbc_stricmp(name, "i16") == 0) return FBC_T_I16;
    if (fbc_stricmp(name, "i32") == 0) return FBC_T_I32;
    if (fbc_stricmp(name, "i64") == 0) return FBC_T_I64;
    if (fbc_stricmp(name, "u8") == 0) return FBC_T_U8;
    if (fbc_stricmp(name, "u16") == 0) return FBC_T_U16;
    if (fbc_stricmp(name, "u32") == 0) return FBC_T_U32;
    if (fbc_stricmp(name, "u64") == 0) return FBC_T_U64;
    if (fbc_stricmp(name, "hex") == 0) return FBC_T_HEX;
    if (fbc_stricmp(name, "oct") == 0) return FBC_T_OCT;
    if (fbc_stricmp(name, "f32") == 0) return FBC_T_F32;
    if (fbc_stricmp(name, "f64") == 0) return FBC_T_F64;
    if (fbc_stricmp(name, "cstr") == 0) return FBC_T_CSTR;
    if (fbc_stricmp(name, "char") == 0 || fbc_stricmp(name, "ch") == 0) return FBC_T_CHAR;
    return FBC_T_UNKNOWN;
}

const char *fossil_bluecrab_type_to_string(fossil_bluecrab_type_t tag) {
    switch (tag) {
        case FBC_T_I8:  return "i8";
        case FBC_T_I16: return "i16";
        case FBC_T_I32: return "i32";
        case FBC_T_I64: return "i64";
        case FBC_T_U8:  return "u8";
        case FBC_T_U16: return "u16";
        case FBC_T_U32: return "u32";
        case FBC_T_U64: return "u64";
        case FBC_T_HEX: return "hex";
        case FBC_T_OCT: return "oct";
        case FBC_T_F32: return "f32";
        case FBC_T_F64: return "f64";
        case FBC_T_CSTR: return "cstr";
        case FBC_T_CHAR: return "char";
        default: return "unknown";
    }
}

/* ---------- safety helpers for str->int conversions ---------- */
static bool fbc_str_to_int64(const char *s, long long *out, int base) {
    if (!s || !out) return false;
    errno = 0;
    char *end = NULL;
    long long val = strtoll(s, &end, base);
    if (end == s) return false; /* no conversion */
    if (errno == ERANGE) return false;
    /* skip trailing spaces */
    while (*end && isspace((unsigned char)*end)) end++;
    if (*end != '\0') return false; /* unparsed trailing */
    *out = val;
    return true;
}
static bool fbc_str_to_uint64(const char *s, unsigned long long *out, int base) {
    if (!s || !out) return false;
    errno = 0;
    char *end = NULL;
    unsigned long long val = strtoull(s, &end, base);
    if (end == s) return false;
    if (errno == ERANGE) return false;
    while (*end && isspace((unsigned char)*end)) end++;
    if (*end != '\0') return false;
    *out = val;
    return true;
}
static bool fbc_str_to_float(const char *s, float *out) {
    if (!s || !out) return false;
    errno = 0;
    char *end = NULL;
    float v = strtof(s, &end);
    if (end == s) return false;
    if (errno == ERANGE) return false;
    while (*end && isspace((unsigned char)*end)) end++;
    if (*end != '\0') return false;
    *out = v;
    return true;
}
static bool fbc_str_to_double(const char *s, double *out) {
    if (!s || !out) return false;
    errno = 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s) return false;
    if (errno == ERANGE) return false;
    while (*end && isspace((unsigned char)*end)) end++;
    if (*end != '\0') return false;
    *out = v;
    return true;
}

/* ---------- parse function ---------- */
bool fossil_bluecrab_parse(fossil_bluecrab_type_t tag, const char *str, fossil_bluecrab_value_t *out) {
    if (!str || !out) return false;
    memset(out, 0, sizeof(*out));

    switch (tag) {
        case FBC_T_I8: {
            long long v;
            if (!fbc_str_to_int64(str, &v, 10)) return false;
            if (v < INT8_MIN || v > INT8_MAX) return false;
            out->i8 = (int8_t)v;
            return true;
        }
        case FBC_T_I16: {
            long long v;
            if (!fbc_str_to_int64(str, &v, 10)) return false;
            if (v < INT16_MIN || v > INT16_MAX) return false;
            out->i16 = (int16_t)v;
            return true;
        }
        case FBC_T_I32: {
            long long v;
            if (!fbc_str_to_int64(str, &v, 10)) return false;
            if (v < INT32_MIN || v > INT32_MAX) return false;
            out->i32 = (int32_t)v;
            return true;
        }
        case FBC_T_I64: {
            long long v;
            if (!fbc_str_to_int64(str, &v, 10)) return false;
            out->i64 = (int64_t)v;
            return true;
        }
        case FBC_T_U8: {
            unsigned long long v;
            if (!fbc_str_to_uint64(str, &v, 10)) return false;
            if (v > UINT8_MAX) return false;
            out->u8 = (uint8_t)v;
            return true;
        }
        case FBC_T_U16: {
            unsigned long long v;
            if (!fbc_str_to_uint64(str, &v, 10)) return false;
            if (v > UINT16_MAX) return false;
            out->u16 = (uint16_t)v;
            return true;
        }
        case FBC_T_U32: {
            unsigned long long v;
            if (!fbc_str_to_uint64(str, &v, 10)) return false;
            if (v > UINT32_MAX) return false;
            out->u32 = (uint32_t)v;
            return true;
        }
        case FBC_T_U64: {
            unsigned long long v;
            if (!fbc_str_to_uint64(str, &v, 10)) return false;
            out->u64 = (uint64_t)v;
            return true;
        }
        case FBC_T_HEX: {
            /* Accept "0x" prefix or not */
            const char *s = str;
            while (isspace((unsigned char)*s)) s++;
            if (s[0]=='0' && (s[1]=='x' || s[1]=='X')) s += 2;
            unsigned long long v;
            if (!fbc_str_to_uint64(s, &v, 16)) return false;
            out->u64 = (uint64_t)v;
            return true;
        }
        case FBC_T_OCT: {
            const char *s = str;
            while (isspace((unsigned char)*s)) s++;
            /* allow optional leading 0 */
            if (s[0]=='0' && (s[1] != 'x' && s[1] != 'X')) s += 0; /* keep '0' because strtoull with base 8 expects digits */
            unsigned long long v;
            if (!fbc_str_to_uint64(s, &v, 8)) return false;
            out->u64 = (uint64_t)v;
            return true;
        }
        case FBC_T_F32: {
            float v;
            if (!fbc_str_to_float(str, &v)) return false;
            out->f32 = v;
            return true;
        }
        case FBC_T_F64: {
            double v;
            if (!fbc_str_to_double(str, &v)) return false;
            out->f64 = v;
            return true;
        }
        case FBC_T_CSTR: {
            /* allocate a copy */
            char *dup = NULL;
            size_t len = strlen(str);
            dup = (char*)malloc(len + 1);
            if (!dup) return false;
            memcpy(dup, str, len + 1);
            out->cstr = dup;
            return true;
        }
        case FBC_T_CHAR: {
            /* accept single character or first char of string; allow quoted like 'a' */
            const char *s = str;
            while (isspace((unsigned char)*s)) s++;
            if (s[0] == '\'' && s[1] && s[2] == '\'') {
                out->ch = s[1];
                return true;
            }
            if (s[0] == '\0') return false;
            out->ch = s[0];
            return true;
        }
        default:
            return false;
    }
}

/* ---------- to string ---------- */
char *fossil_bluecrab_to_string(fossil_bluecrab_type_t tag, const fossil_bluecrab_value_t *val) {
    if (!val) return NULL;
    char *buf = NULL;
    int needed = 0;
    switch (tag) {
        case FBC_T_I8:
            needed = snprintf(NULL, 0, "%" PRId8, val->i8) + 1;
            buf = (char*)malloc(needed);
            if (!buf) return NULL;
            snprintf(buf, needed, "%" PRId8, val->i8);
            return buf;
        case FBC_T_I16:
            needed = snprintf(NULL, 0, "%" PRId16, val->i16) + 1;
            buf = (char*)malloc(needed);
            if (!buf) return NULL;
            snprintf(buf, needed, "%" PRId16, val->i16);
            return buf;
        case FBC_T_I32:
            needed = snprintf(NULL, 0, "%" PRId32, val->i32) + 1;
            buf = (char*)malloc(needed);
            if (!buf) return NULL;
            snprintf(buf, needed, "%" PRId32, val->i32);
            return buf;
        case FBC_T_I64:
            needed = snprintf(NULL, 0, "%" PRId64, val->i64) + 1;
            buf = (char*)malloc(needed);
            if (!buf) return NULL;
            snprintf(buf, needed, "%" PRId64, val->i64);
            return buf;
        case FBC_T_U8:
            needed = snprintf(NULL, 0, "%" PRIu8, val->u8) + 1;
            buf = (char*)malloc(needed);
            if (!buf) return NULL;
            snprintf(buf, needed, "%" PRIu8, val->u8);
            return buf;
        case FBC_T_U16:
            needed = snprintf(NULL, 0, "%" PRIu16, val->u16) + 1;
            buf = (char*)malloc(needed);
            if (!buf) return NULL;
            snprintf(buf, needed, "%" PRIu16, val->u16);
            return buf;
        case FBC_T_U32:
            needed = snprintf(NULL, 0, "%" PRIu32, val->u32) + 1;
            buf = (char*)malloc(needed);
            if (!buf) return NULL;
            snprintf(buf, needed, "%" PRIu32, val->u32);
            return buf;
        case FBC_T_U64:
        case FBC_T_HEX:
        case FBC_T_OCT:
            /* format as decimal for u64 and special forms for hex/oct */
            if (tag == FBC_T_HEX) {
                needed = snprintf(NULL, 0, "0x%" PRIx64, val->u64) + 1;
                buf = (char*)malloc(needed);
                if (!buf) return NULL;
                snprintf(buf, needed, "0x%" PRIx64, val->u64);
                return buf;
            } else if (tag == FBC_T_OCT) {
                needed = snprintf(NULL, 0, "0%" PRIo64, val->u64) + 1;
                buf = (char*)malloc(needed);
                if (!buf) return NULL;
                snprintf(buf, needed, "0%" PRIo64, val->u64);
                return buf;
            } else {
                needed = snprintf(NULL, 0, "%" PRIu64, val->u64) + 1;
                buf = (char*)malloc(needed);
                if (!buf) return NULL;
                snprintf(buf, needed, "%" PRIu64, val->u64);
                return buf;
            }
        case FBC_T_F32: {
            /* use "%g" formatting */
            needed = snprintf(NULL, 0, "%g", (double)val->f32) + 1;
            buf = (char*)malloc(needed);
            if (!buf) return NULL;
            snprintf(buf, needed, "%g", (double)val->f32);
            return buf;
        }
        case FBC_T_F64: {
            needed = snprintf(NULL, 0, "%g", val->f64) + 1;
            buf = (char*)malloc(needed);
            if (!buf) return NULL;
            snprintf(buf, needed, "%g", val->f64);
            return buf;
        }
        case FBC_T_CSTR:
            if (!val->cstr) return NULL;
            buf = (char*)malloc(strlen(val->cstr) + 1);
            if (!buf) return NULL;
            strcpy(buf, val->cstr);
            return buf;
        case FBC_T_CHAR:
            buf = (char*)malloc(2);
            if (!buf) return NULL;
            buf[0] = val->ch;
            buf[1] = '\0';
            return buf;
        default:
            return NULL;
    }
}

/* ---------- free ---------- */
void fossil_bluecrab_free_value(fossil_bluecrab_type_t tag, fossil_bluecrab_value_t *val) {
    if (!val) return;
    if (tag == FBC_T_CSTR && val->cstr) {
        free(val->cstr);
        val->cstr = NULL;
    }
    /* zero out for safety */
    memset(val, 0, sizeof(*val));
}

/* ---------- equal ---------- */
#include <string.h>
bool fossil_bluecrab_equal(fossil_bluecrab_type_t tag, const fossil_bluecrab_value_t *a, const fossil_bluecrab_value_t *b) {
    if (!a || !b) return false;
    switch (tag) {
        case FBC_T_I8:  return a->i8 == b->i8;
        case FBC_T_I16: return a->i16 == b->i16;
        case FBC_T_I32: return a->i32 == b->i32;
        case FBC_T_I64: return a->i64 == b->i64;
        case FBC_T_U8:  return a->u8 == b->u8;
        case FBC_T_U16: return a->u16 == b->u16;
        case FBC_T_U32: return a->u32 == b->u32;
        case FBC_T_U64:
        case FBC_T_HEX:
        case FBC_T_OCT:
            return a->u64 == b->u64;
        case FBC_T_F32: {
            /* bitwise compare to avoid -0 vs +0 weirdness; use memcmp on float */
            uint32_t A, B;
            memcpy(&A, &a->f32, sizeof(A));
            memcpy(&B, &b->f32, sizeof(B));
            return A == B;
        }
        case FBC_T_F64: {
            uint64_t A, B;
            memcpy(&A, &a->f64, sizeof(A));
            memcpy(&B, &b->f64, sizeof(B));
            return A == B;
        }
        case FBC_T_CSTR:
            if (a->cstr == NULL && b->cstr == NULL) return true;
            if (a->cstr == NULL || b->cstr == NULL) return false;
            return strcmp(a->cstr, b->cstr) == 0;
        case FBC_T_CHAR:
            return a->ch == b->ch;
        default:
            return false;
    }
}

/* ---------- 64-bit hash with salt/time (only 1 param allowed) ---------- */
/* We'll implement FNV-1a 64-bit, then mix with a runtime salt (derived from time() and
 * a pointer/address entropy and optionally pid if available).
 *
 * The API signature is exactly: static uint64_t fossil_bluecrab_hash(const char *str)
 *
 * Salt is computed once (on first call) and stored in static variable so repeated calls
 * in the same process will be consistent; salt includes time() so it's time-dependent.
 */

static uint64_t fbc_runtime_salt(void) {
    /* combine time, pointer, pid (if available), and monotonic clock if available */
    static uint64_t salt = 0;
    if (salt != 0) return salt;

    uint64_t s = 0xcbf29ce484222325ULL; /* FNV offset basis as starting entropy */

    time_t t = time(NULL);
    s ^= (uint64_t)(t);
    s *= 0x100000001b3ULL;

    /* add address entropy */
    uintptr_t p = (uintptr_t)&fbc_runtime_salt;
    s ^= (uint64_t)p;
    s *= 0x100000001b3ULL;

    /* try to add pid on POSIX */
#ifdef __unix__
    s ^= (uint64_t)(uintptr_t)getpid();
    s *= 0x100000001b3ULL;
#endif

    /* mix once with a few rounds of xorshift */
    s ^= (s << 13);
    s ^= (s >> 7);
    s ^= (s << 17);

    /* avoid zero salt */
    if (s == 0) s = 0x9e3779b97f4a7c15ULL;

    salt = s;
    return salt;
}

static uint64_t fbc_fnv1a_64(const unsigned char *data, size_t len) {
    const uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;
    const uint64_t FNV_PRIME = 0x100000001b3ULL;
    uint64_t h = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < len; ++i) {
        h ^= (uint64_t)data[i];
        h *= FNV_PRIME;
    }
    return h;
}

static uint64_t fbc_rotl64(uint64_t x, unsigned r) {
    return (x << r) | (x >> (64 - r));
}

static uint64_t fbc_mix(uint64_t h, uint64_t salt) {
    /* mix using xor/rol/multiply */
    h ^= salt;
    h = fbc_rotl64(h, 31);
    h *= 0x9ddfea08eb382d69ULL; /* large odd constant */
    h ^= (h >> 33);
    h *= 0xff51afd7ed558ccdULL;
    h ^= (h >> 33);
    return h;
}

/* public: exactly required signature */
static uint64_t fossil_bluecrab_hash(const char *str) {
    if (!str) return 0;
    uint64_t salt = fbc_runtime_salt();
    const unsigned char *data = (const unsigned char*)str;
    size_t len = strlen(str);
    uint64_t h = fbc_fnv1a_64(data, len);
    h = fbc_mix(h, salt);
    return h;
}
