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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

/* --- creation helpers --- */

void fossil_bluecrab_init(fossil_bluecrab_t *out) {
    if (!out) return;
    out->type = FBC_TYPE_NONE;
    out->v.cstr = NULL;
}

void fossil_bluecrab_set_i64(fossil_bluecrab_t *out, int64_t val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_I64;
    out->v.i64 = val;
}
void fossil_bluecrab_set_i32(fossil_bluecrab_t *out, int32_t val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_I32;
    out->v.i32 = val;
}
void fossil_bluecrab_set_i16(fossil_bluecrab_t *out, int16_t val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_I16;
    out->v.i16 = val;
}
void fossil_bluecrab_set_i8(fossil_bluecrab_t *out, int8_t val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_I8;
    out->v.i8 = val;
}

void fossil_bluecrab_set_u64(fossil_bluecrab_t *out, uint64_t val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_U64;
    out->v.u64 = val;
}
void fossil_bluecrab_set_u32(fossil_bluecrab_t *out, uint32_t val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_U32;
    out->v.u32 = val;
}
void fossil_bluecrab_set_u16(fossil_bluecrab_t *out, uint16_t val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_U16;
    out->v.u16 = val;
}
void fossil_bluecrab_set_u8(fossil_bluecrab_t *out, uint8_t val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_U8;
    out->v.u8 = val;
}

void fossil_bluecrab_set_hex(fossil_bluecrab_t *out, uint64_t val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_HEX;
    out->v.u64 = val;
}
void fossil_bluecrab_set_oct(fossil_bluecrab_t *out, uint64_t val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_OCT;
    out->v.u64 = val;
}

void fossil_bluecrab_set_f64(fossil_bluecrab_t *out, double val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_F64;
    out->v.f64 = val;
}
void fossil_bluecrab_set_f32(fossil_bluecrab_t *out, float val) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_F32;
    out->v.f32 = val;
}

/* Duplicate the string (malloc+strcpy) */
void fossil_bluecrab_set_cstr_dup(fossil_bluecrab_t *out, const char *s) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_CSTR;
    if (s) {
        size_t n = strlen(s) + 1;
        char *dup = (char*)malloc(n);
        if (dup) memcpy(dup, s, n);
        out->v.cstr = dup;
    } else {
        out->v.cstr = NULL;
    }
}

/* Take pointer as-is (alias) -- caller manages lifetime */
void fossil_bluecrab_set_cstr_alias(fossil_bluecrab_t *out, char *s) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_CSTR;
    out->v.cstr = s;
}

void fossil_bluecrab_set_char(fossil_bluecrab_t *out, char c) {
    fossil_bluecrab_clear(out);
    out->type = FBC_TYPE_CHAR;
    out->v.ch = c;
}

/* Clear (free duplicated string only) */
void fossil_bluecrab_clear(fossil_bluecrab_t *v) {
    if (!v) return;
    if (v->type == FBC_TYPE_CSTR && v->v.cstr) {
        free(v->v.cstr);
        v->v.cstr = NULL;
    }
    v->type = FBC_TYPE_NONE;
}

/* --- mapping functions --- */

const char *fossil_bluecrab_type_to_string(fossil_bluecrab_type_t t) {
    switch (t) {
        case FBC_TYPE_NONE: return "none";
        case FBC_TYPE_I8:   return "i8";
        case FBC_TYPE_I16:  return "i16";
        case FBC_TYPE_I32:  return "i32";
        case FBC_TYPE_I64:  return "i64";
        case FBC_TYPE_U8:   return "u8";
        case FBC_TYPE_U16:  return "u16";
        case FBC_TYPE_U32:  return "u32";
        case FBC_TYPE_U64:  return "u64";
        case FBC_TYPE_HEX:  return "hex";
        case FBC_TYPE_OCT:  return "oct";
        case FBC_TYPE_F32:  return "f32";
        case FBC_TYPE_F64:  return "f64";
        case FBC_TYPE_CSTR: return "cstr";
        case FBC_TYPE_CHAR: return "char";
        default: return "unknown";
    }
}

fossil_bluecrab_type_t fossil_bluecrab_string_to_type(const char *s) {
    if (!s) return FBC_TYPE_NONE;
    /* simple, case-sensitive matches */
    if (strcmp(s,"i8")==0) return FBC_TYPE_I8;
    if (strcmp(s,"i16")==0) return FBC_TYPE_I16;
    if (strcmp(s,"i32")==0) return FBC_TYPE_I32;
    if (strcmp(s,"i64")==0) return FBC_TYPE_I64;
    if (strcmp(s,"u8")==0) return FBC_TYPE_U8;
    if (strcmp(s,"u16")==0) return FBC_TYPE_U16;
    if (strcmp(s,"u32")==0) return FBC_TYPE_U32;
    if (strcmp(s,"u64")==0) return FBC_TYPE_U64;
    if (strcmp(s,"hex")==0) return FBC_TYPE_HEX;
    if (strcmp(s,"oct")==0) return FBC_TYPE_OCT;
    if (strcmp(s,"f32")==0) return FBC_TYPE_F32;
    if (strcmp(s,"f64")==0) return FBC_TYPE_F64;
    if (strcmp(s,"cstr")==0) return FBC_TYPE_CSTR;
    if (strcmp(s,"char")==0) return FBC_TYPE_CHAR;
    if (strcmp(s,"none")==0) return FBC_TYPE_NONE;
    return FBC_TYPE_NONE;
}

/* --- to-string conversion --- */
/* Writes a textual representation into buf (NUL-terminated). Returns bytes written (excluding NUL). */
size_t fossil_bluecrab_to_string(const fossil_bluecrab_t *v, char *buf, size_t bufsize) {
    if (!v || !buf || bufsize == 0) return 0;
    int wrote = 0;
    switch (v->type) {
        case FBC_TYPE_I8:
            wrote = snprintf(buf, bufsize, "%" PRId8, v->v.i8);
            break;
        case FBC_TYPE_I16:
            wrote = snprintf(buf, bufsize, "%" PRId16, v->v.i16);
            break;
        case FBC_TYPE_I32:
            wrote = snprintf(buf, bufsize, "%" PRId32, v->v.i32);
            break;
        case FBC_TYPE_I64:
            wrote = snprintf(buf, bufsize, "%" PRId64, v->v.i64);
            break;
        case FBC_TYPE_U8:
            wrote = snprintf(buf, bufsize, "%" PRIu8, v->v.u8);
            break;
        case FBC_TYPE_U16:
            wrote = snprintf(buf, bufsize, "%" PRIu16, v->v.u16);
            break;
        case FBC_TYPE_U32:
            wrote = snprintf(buf, bufsize, "%" PRIu32, v->v.u32);
            break;
        case FBC_TYPE_U64:
            wrote = snprintf(buf, bufsize, "%" PRIu64, v->v.u64);
            break;
        case FBC_TYPE_HEX:
            wrote = snprintf(buf, bufsize, "0x%" PRIx64, v->v.u64);
            break;
        case FBC_TYPE_OCT:
            wrote = snprintf(buf, bufsize, "0%" PRIo64, v->v.u64);
            break;
        case FBC_TYPE_F32:
            wrote = snprintf(buf, bufsize, "%g", (double)v->v.f32);
            break;
        case FBC_TYPE_F64:
            wrote = snprintf(buf, bufsize, "%g", v->v.f64);
            break;
        case FBC_TYPE_CSTR:
            if (v->v.cstr) {
                wrote = snprintf(buf, bufsize, "%s", v->v.cstr);
            } else {
                wrote = snprintf(buf, bufsize, "(null)");
            }
            break;
        case FBC_TYPE_CHAR:
            /* printable char or escaped */
            if ((unsigned char)v->v.ch >= 0x20 && (unsigned char)v->v.ch <= 0x7e) {
                wrote = snprintf(buf, bufsize, "%c", v->v.ch);
            } else {
                wrote = snprintf(buf, bufsize, "\\x%02x", (unsigned char)v->v.ch);
            }
            break;
        case FBC_TYPE_NONE:
        default:
            wrote = snprintf(buf, bufsize, "(none)");
            break;
    }
    if (wrote < 0) return 0;
    if ((size_t)wrote >= bufsize) return bufsize - 1; /* truncated */
    return (size_t)wrote;
}

/* --- 64-bit hash --- */
/* We implement a simple but well-mixed 64-bit hash:
 * - initial h = salt ^ (time + golden_constant)
 * - for each 8-byte word process; remaining bytes processed one-by-one
 * - multiplies and xors with mixing constants (splitmix-style constants)
 *
 * This is NOT intended as a drop-in cryptographic MAC. If you need cryptographically
 * strong HMAC, use an HMAC-SHA-256 truncated to 64 bits or SipHash.
 */
static inline uint64_t rotl64(uint64_t x, int r) {
    return (x << r) | (x >> (64 - r));
}

uint64_t fossil_bluecrab_hash64(const void *data, size_t len, uint64_t salt, uint64_t t) {
    const uint8_t *p = (const uint8_t*)data;
    const uint64_t GOLDEN = 0x9e3779b97f4a7c15ULL;
    uint64_t h = salt ^ (t + GOLDEN);

    /* Process 8-byte chunks */
    while (len >= 8) {
        uint64_t k = 0;
        memcpy(&k, p, 8);
        /* mix */
        k ^= h;
        k *= 0xbf58476d1ce4e5b9ULL;
        k = rotl64(k, 31);
        k *= 0x94d049bb133111ebULL;
        h ^= k;
        h = rotl64(h, 27) * 5 + 0x52dce729;
        p += 8;
        len -= 8;
    }

    /* tail */
    uint64_t tail = 0;
    switch (len) {
        case 7: tail ^= (uint64_t)p[6] << 48; /* fallthrough */
        case 6: tail ^= (uint64_t)p[5] << 40; /* fallthrough */
        case 5: tail ^= (uint64_t)p[4] << 32; /* fallthrough */
        case 4: tail ^= (uint64_t)p[3] << 24; /* fallthrough */
        case 3: tail ^= (uint64_t)p[2] << 16; /* fallthrough */
        case 2: tail ^= (uint64_t)p[1] << 8;  /* fallthrough */
        case 1: tail ^= (uint64_t)p[0];       break;
        case 0: break;
    }
    if (tail) {
        tail ^= h;
        tail *= 0xbf58476d1ce4e5b9ULL;
        tail = rotl64(tail, 31);
        tail *= 0x94d049bb133111ebULL;
        h ^= tail;
    }

    /* finalization mix (inspired by splitmix64) */
    h ^= h >> 30;
    h *= 0xbf58476d1ce4e5b9ULL;
    h ^= h >> 27;
    h *= 0x94d049bb133111ebULL;
    h ^= h >> 31;

    return h;
}

/* Serialize value into a small buffer and hash it. This keeps hash_value deterministic across types. */
uint64_t fossil_bluecrab_hash_value(const fossil_bluecrab_t *v, uint64_t salt, uint64_t t) {
    if (!v) return fossil_bluecrab_hash64(NULL, 0, salt, t);
    uint8_t buf[256];
    size_t pos = 0;

    /* include type tag in serialization */
    if (pos + 1 <= sizeof(buf)) {
        buf[pos++] = (uint8_t)(v->type & 0xff);
    }

    switch (v->type) {
        case FBC_TYPE_I8:
            if (pos + sizeof(int8_t) <= sizeof(buf)) {
                memcpy(buf + pos, &v->v.i8, sizeof(int8_t));
                pos += sizeof(int8_t);
            }
            break;
        case FBC_TYPE_I16:
            if (pos + sizeof(int16_t) <= sizeof(buf)) {
                memcpy(buf + pos, &v->v.i16, sizeof(int16_t));
                pos += sizeof(int16_t);
            }
            break;
        case FBC_TYPE_I32:
            if (pos + sizeof(int32_t) <= sizeof(buf)) {
                memcpy(buf + pos, &v->v.i32, sizeof(int32_t));
                pos += sizeof(int32_t);
            }
            break;
        case FBC_TYPE_I64:
            if (pos + sizeof(int64_t) <= sizeof(buf)) {
                memcpy(buf + pos, &v->v.i64, sizeof(int64_t));
                pos += sizeof(int64_t);
            }
            break;
        case FBC_TYPE_U8:
            if (pos + sizeof(uint8_t) <= sizeof(buf)) {
                memcpy(buf + pos, &v->v.u8, sizeof(uint8_t));
                pos += sizeof(uint8_t);
            }
            break;
        case FBC_TYPE_U16:
            if (pos + sizeof(uint16_t) <= sizeof(buf)) {
                memcpy(buf + pos, &v->v.u16, sizeof(uint16_t));
                pos += sizeof(uint16_t);
            }
            break;
        case FBC_TYPE_U32:
            if (pos + sizeof(uint32_t) <= sizeof(buf)) {
                memcpy(buf + pos, &v->v.u32, sizeof(uint32_t));
                pos += sizeof(uint32_t);
            }
            break;
        case FBC_TYPE_U64:
        case FBC_TYPE_HEX:
        case FBC_TYPE_OCT:
            if (pos + sizeof(uint64_t) <= sizeof(buf)) {
                memcpy(buf + pos, &v->v.u64, sizeof(uint64_t));
                pos += sizeof(uint64_t);
            }
            break;
        case FBC_TYPE_F32:
            if (pos + sizeof(float) <= sizeof(buf)) {
                memcpy(buf + pos, &v->v.f32, sizeof(float));
                pos += sizeof(float);
            }
            break;
        case FBC_TYPE_F64:
            if (pos + sizeof(double) <= sizeof(buf)) {
                memcpy(buf + pos, &v->v.f64, sizeof(double));
                pos += sizeof(double);
            }
            break;
        case FBC_TYPE_CSTR:
            if (v->v.cstr) {
                size_t n = strlen(v->v.cstr);
                /* copy up to remaining buffer */
                size_t tocopy = (n <= sizeof(buf) - pos) ? n : (sizeof(buf) - pos);
                if (tocopy) {
                    memcpy(buf + pos, v->v.cstr, tocopy);
                    pos += tocopy;
                }
            }
            break;
        case FBC_TYPE_CHAR:
            if (pos + 1 <= sizeof(buf)) {
                buf[pos++] = (uint8_t)v->v.ch;
            }
            break;
        case FBC_TYPE_NONE:
        default:
            /* nothing else */
            break;
    }

    return fossil_bluecrab_hash64(buf, pos, salt, t);
}
