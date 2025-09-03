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
#ifndef FOSSIL_CRABDB_CORE_H
#define FOSSIL_CRABDB_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>   // for file size
#include <errno.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Tag for the union */
typedef enum {
    FBC_TYPE_NONE = 0,
    FBC_TYPE_I8,
    FBC_TYPE_I16,
    FBC_TYPE_I32,
    FBC_TYPE_I64,
    FBC_TYPE_U8,
    FBC_TYPE_U16,
    FBC_TYPE_U32,
    FBC_TYPE_U64,
    FBC_TYPE_HEX,   /* stored as u64, printed in hex */
    FBC_TYPE_OCT,   /* stored as u64, printed in octal */
    FBC_TYPE_F32,
    FBC_TYPE_F64,
    FBC_TYPE_CSTR,  /* NUL-terminated char*; ownership documented on functions */
    FBC_TYPE_CHAR   /* single char */
} fossil_bluecrab_type_t;

/* The union value */
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
    char    *cstr; /* pointer; ownership managed by caller (see helpers below) */
    char     ch;
} fossil_bluecrab_value_t;

/* Container holding the tag + value */
typedef struct {
    fossil_bluecrab_type_t type;
    fossil_bluecrab_value_t v;
} fossil_bluecrab_t;

/* Creation helpers */
void fossil_bluecrab_init(fossil_bluecrab_t *out);
void fossil_bluecrab_set_i64(fossil_bluecrab_t *out, int64_t val);
void fossil_bluecrab_set_i32(fossil_bluecrab_t *out, int32_t val);
void fossil_bluecrab_set_i16(fossil_bluecrab_t *out, int16_t val);
void fossil_bluecrab_set_i8 (fossil_bluecrab_t *out, int8_t  val);
void fossil_bluecrab_set_u64(fossil_bluecrab_t *out, uint64_t val);
void fossil_bluecrab_set_u32(fossil_bluecrab_t *out, uint32_t val);
void fossil_bluecrab_set_u16(fossil_bluecrab_t *out, uint16_t val);
void fossil_bluecrab_set_u8 (fossil_bluecrab_t *out, uint8_t  val);
void fossil_bluecrab_set_hex(fossil_bluecrab_t *out, uint64_t val);
void fossil_bluecrab_set_oct(fossil_bluecrab_t *out, uint64_t val);
void fossil_bluecrab_set_f64(fossil_bluecrab_t *out, double val);
void fossil_bluecrab_set_f32(fossil_bluecrab_t *out, float val);
void fossil_bluecrab_set_cstr_dup(fossil_bluecrab_t *out, const char *s); /* duplicates string */
void fossil_bluecrab_set_cstr_alias(fossil_bluecrab_t *out, char *s); /* takes pointer, no dup */
void fossil_bluecrab_set_char(fossil_bluecrab_t *out, char c);

/* Cleanup (frees duplicated strings) */
void fossil_bluecrab_clear(fossil_bluecrab_t *v);

/* Type <-> string mapping */
const char *fossil_bluecrab_type_to_string(fossil_bluecrab_type_t t);
fossil_bluecrab_type_t fossil_bluecrab_string_to_type(const char *s);

/* Conversion to string (writes into user buffer) */
size_t fossil_bluecrab_to_string(const fossil_bluecrab_t *v, char *buf, size_t bufsize);

/* Hashing: 64-bit hash mixing salt and time.
 * - data: pointer to data
 * - len: byte length
 * - salt: user-supplied salt (may be 0)
 * - t: unix time (seconds). Use time(NULL) if you want "now".
 *
 * Returns 64-bit hash value.
 */
uint64_t fossil_bluecrab_hash64(const void *data, size_t len, uint64_t salt, uint64_t t);

/* Convenience: hash a fossil_bluecrab_t's serialized representation */
uint64_t fossil_bluecrab_hash_value(const fossil_bluecrab_t *v, uint64_t salt, uint64_t t);

#ifdef __cplusplus
}
#include <string>
#include <vector>
#include <algorithm>

namespace fossil {

    namespace bluecrab {
        /**
         * @brief FileShell class provides static methods for file operations.
         *
         * This class wraps the C API for file operations, providing a C++ interface
         * using std::string and std::vector. All methods are static and do not require
         * instantiation.
         */
        class FileShell {
        public:
            /**
             * @brief Writes text data to a file (overwrites if exists).
             *
             * @param path  Path to the file.
             * @param data  String data to write.
             * @return      true on success, false on failure.
             */
            static bool write(const std::string& path, const std::string& data) {
                return fossil_bluecrab_fileshell_write(path.c_str(), data.c_str());
            }

            /**
             * @brief Appends text data to a file (creates if missing).
             *
             * @param path  Path to the file.
             * @param data  String data to append.
             * @return      true on success, false on failure.
             */
            static bool append(const std::string& path, const std::string& data) {
                return fossil_bluecrab_fileshell_append(path.c_str(), data.c_str());
            }

            /**
             * @brief Reads text data from a file.
             *
             * @param path  Path to the file.
             * @param out   Output string to store file contents.
             * @return      true on success, false on failure.
             */
            static bool read(const std::string& path, std::string& out) {
                long sz = fossil_bluecrab_fileshell_size(path.c_str());
                if (sz < 0) return false;
                std::string buf(sz, '\0');
                bool ok = fossil_bluecrab_fileshell_read(path.c_str(), &buf[0], sz + 1);
                if (ok) {
                    out = buf.c_str();
                }
                return ok;
            }

            /**
             * @brief Deletes a file.
             *
             * @param path  Path to the file.
             * @return      true if deleted, false otherwise.
             */
            static bool remove(const std::string& path) {
                return fossil_bluecrab_fileshell_delete(path.c_str());
            }

            /**
             * @brief Checks if a file exists.
             *
             * @param path  Path to the file.
             * @return      true if file exists, false otherwise.
             */
            static bool exists(const std::string& path) {
                return fossil_bluecrab_fileshell_exists(path.c_str());
            }

            /**
             * @brief Returns the file size in bytes.
             *
             * @param path  Path to the file.
             * @return      File size in bytes, or -1 on error.
             */
            static long size(const std::string& path) {
                return fossil_bluecrab_fileshell_size(path.c_str());
            }

            /**
             * @brief Lists files in a directory.
             *
             * @param dir_path  Path to the directory.
             * @param files     Output vector to store file names.
             * @param max_files Maximum number of files to list.
             * @return          Number of files listed, or -1 on error.
             */
            static int list(const std::string& dir_path, std::vector<std::string>& files, size_t max_files) {
                std::vector<char*> c_files(max_files, nullptr);
                int count = fossil_bluecrab_fileshell_list(dir_path.c_str(), c_files.data(), max_files);
                if (count > 0) {
                    files.clear();
                    for (int i = 0; i < count; ++i) {
                        if (c_files[i]) {
                            files.emplace_back(c_files[i]);
                            free(c_files[i]);
                        }
                    }
                }
                return count;
            }
        };

    } // namespace bluecrab

} // namespace fossil

#endif

#endif /* FOSSIL_CRABDB_FRAMEWORK_H */
