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
#include <fossil/unittest/framework.h> // Includes the Fossil Unit Test Framework
#include <fossil/mockup/framework.h>   // Includes the Fossil Mockup Framework
#include <fossil/unittest/assume.h>            // Includes the Fossil Assume Framework

#include "fossil/crabdb/framework.h"


// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Blue CrabDB Database
// * * * * * * * * * * * * * * * * * * * * * * * *

FOSSIL_TEST(test_fossil_crabdb_create) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    ASSUME_NOT_CNULL(db);
    ASSUME_ITS_EQUAL_I32(0, db->node_count);
    fossil_crabdb_destroy(db);
}

FOSSIL_TEST(test_fossil_crabdb_destroy) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    fossil_crabdb_destroy(db); // Ensure no crash or memory leak
}

FOSSIL_TEST(test_fossil_crabdb_insert) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    bool result = fossil_crabdb_insert(db, "key1", "value1", FOSSIL_CRABDB_TYPE_STRING);
    ASSUME_ITS_TRUE(result);
    ASSUME_ITS_EQUAL_I32(1, db->node_count);
    fossil_crabdb_destroy(db);
}

FOSSIL_TEST(test_fossil_crabdb_insert_duplicate_key) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    fossil_crabdb_insert(db, "key1", "value1", FOSSIL_CRABDB_TYPE_STRING);
    bool result = fossil_crabdb_insert(db, "key1", "value2", FOSSIL_CRABDB_TYPE_STRING); // Should fail or update
    ASSUME_ITS_TRUE(result); // Assuming it updates; change to ASSUME_ITS_FALSE if not.
    ASSUME_ITS_EQUAL_I32(1, db->node_count);
    char value[FOSSIL_CRABDB_VAL_SIZE];
    fossil_crabdb_select(db, "key1", value, sizeof(value));
    ASSUME_ITS_EQUAL_CSTR("value2", value); // Verify updated value
    fossil_crabdb_destroy(db);
}

FOSSIL_TEST(test_fossil_crabdb_select_existing_key) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    fossil_crabdb_insert(db, "key1", "value1", FOSSIL_CRABDB_TYPE_STRING);
    char value[FOSSIL_CRABDB_VAL_SIZE];
    bool result = fossil_crabdb_select(db, "key1", value, sizeof(value));
    ASSUME_ITS_TRUE(result);
    ASSUME_ITS_EQUAL_CSTR("value1", value);
    fossil_crabdb_destroy(db);
}

FOSSIL_TEST(test_fossil_crabdb_select_non_existing_key) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    char value[FOSSIL_CRABDB_VAL_SIZE];
    bool result = fossil_crabdb_select(db, "non_existing_key", value, sizeof(value));
    ASSUME_ITS_FALSE(result); // Should return false
    fossil_crabdb_destroy(db);
}

FOSSIL_TEST(test_fossil_crabdb_update) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    fossil_crabdb_insert(db, "key1", "value1", FOSSIL_CRABDB_TYPE_STRING);
    bool result = fossil_crabdb_update(db, "key1", "value_updated");
    ASSUME_ITS_TRUE(result);
    char value[FOSSIL_CRABDB_VAL_SIZE];
    fossil_crabdb_select(db, "key1", value, sizeof(value));
    ASSUME_ITS_EQUAL_CSTR("value_updated", value);
    fossil_crabdb_destroy(db);
}

FOSSIL_TEST(test_fossil_crabdb_delete) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    fossil_crabdb_insert(db, "key1", "value1", FOSSIL_CRABDB_TYPE_STRING);
    bool result = fossil_crabdb_delete(db, "key1");
    ASSUME_ITS_TRUE(result);
    ASSUME_ITS_EQUAL_I32(0, db->node_count);
    fossil_crabdb_destroy(db);
}

FOSSIL_TEST(test_fossil_crabdb_delete_non_existing_key) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    bool result = fossil_crabdb_delete(db, "non_existing_key");
    ASSUME_ITS_FALSE(result); // Should return false
    fossil_crabdb_destroy(db);
}

FOSSIL_TEST(test_fossil_crabdb_batch_insert) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    const char keys[][FOSSIL_CRABDB_KEY_SIZE] = {"key1", "key2"};
    const char values[][FOSSIL_CRABDB_VAL_SIZE] = {"value1", "value2"};
    fossil_crabdb_type_t types[] = {FOSSIL_CRABDB_TYPE_STRING, FOSSIL_CRABDB_TYPE_STRING};
    bool result = fossil_crabdb_batch_insert(db, keys, values, types, 2);
    ASSUME_ITS_TRUE(result);
    ASSUME_ITS_EQUAL_I32(2, db->node_count);
    fossil_crabdb_destroy(db);
}

FOSSIL_TEST(test_fossil_crabdb_batch_delete) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    fossil_crabdb_insert(db, "key1", "value1", FOSSIL_CRABDB_TYPE_STRING);
    fossil_crabdb_insert(db, "key2", "value2", FOSSIL_CRABDB_TYPE_STRING);
    const char keys[][FOSSIL_CRABDB_KEY_SIZE] = {"key1", "key2"};
    bool result = fossil_crabdb_batch_delete(db, keys, 2);
    ASSUME_ITS_TRUE(result);
    ASSUME_ITS_EQUAL_I32(0, db->node_count);
    fossil_crabdb_destroy(db);
}

FOSSIL_TEST(test_fossil_crabdb_cleanup_expired) {
    fossil_crabdb_t* db = fossil_crabdb_create();
    fossil_crabdb_insert_with_ttl(db, "key1", "value1", FOSSIL_CRABDB_TYPE_STRING, 1); // 1 second TTL
    sleep(2); // Sleep to allow expiration
    bool result = fossil_crabdb_cleanup_expired(db);
    ASSUME_ITS_TRUE(result);
    ASSUME_ITS_EQUAL_I32(0, db->node_count);
    fossil_crabdb_destroy(db);
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *

FOSSIL_TEST_GROUP(c_crab_database_tests) {    
    ADD_TEST(test_fossil_crabdb_create);
    ADD_TEST(test_fossil_crabdb_destroy);
    ADD_TEST(test_fossil_crabdb_insert);
    ADD_TEST(test_fossil_crabdb_insert_duplicate_key);
    ADD_TEST(test_fossil_crabdb_select_existing_key);
    ADD_TEST(test_fossil_crabdb_select_non_existing_key);
    ADD_TEST(test_fossil_crabdb_update);
    ADD_TEST(test_fossil_crabdb_delete);
    ADD_TEST(test_fossil_crabdb_delete_non_existing_key);
    ADD_TEST(test_fossil_crabdb_batch_insert);
    ADD_TEST(test_fossil_crabdb_batch_delete);
    ADD_TEST(test_fossil_crabdb_cleanup_expired);
} // end of tests
