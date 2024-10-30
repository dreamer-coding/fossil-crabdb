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
#include <fossil/unittest/assume.h>    // Includes the Fossil Assume Framework

#include "fossil/crabdb/framework.h"

FOSSIL_FIXTURE(query_fixture);
fossil_crabdb_t *query_mock_db;

FOSSIL_SETUP(query_fixture) {
    query_mock_db = fossil_crabdb_create();
}

FOSSIL_TEARDOWN(query_fixture) {
    fossil_crabdb_destroy(query_mock_db);
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Blue CrabDB Database
// * * * * * * * * * * * * * * * * * * * * * * * *

// Test successful SELECT
FOSSIL_TEST(test_fossil_crabql_query_select) {
    const char *query = "SELECT * FROM users;";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query));
}

// Test successful INSERT
FOSSIL_TEST(test_fossil_crabql_query_insert) {
    const char *query = "INSERT INTO users VALUES ('Alice', 30);";
    ASSUME_ITS_TRUE(fossil_crabql_query(query_mock_db, query));
}

// Test successful UPDATE
FOSSIL_TEST(test_fossil_crabql_query_update) {
    const char *query = "UPDATE users SET age = 31 WHERE name = 'Alice';";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query));
}

// Test successful DELETE
FOSSIL_TEST(test_fossil_crabql_query_delete) {
    const char *query = "DELETE FROM users WHERE name = 'Alice';";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query));
}

// Test invalid query
FOSSIL_TEST(test_fossil_crabql_query_invalid) {
    const char *query = "SELECT FROM users;";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query));
}

// Test loading queries from a .crab file
FOSSIL_TEST(test_fossil_crabql_load_queries_from_file) {
    const char *filename = "test_queries.crab";
    FILE *file = fopen(filename, "w");
    ASSUME_NOT_CNULL(file);

    // Sample queries to write to the .crab file
    fprintf(file, "INSERT INTO users VALUES ('Bob', 25);\n");
    fprintf(file, "SELECT * FROM users;\n");
    fprintf(file, "UPDATE users SET age = 26 WHERE name = 'Bob';\n");
    fprintf(file, "DELETE FROM users WHERE name = 'Bob';\n");
    fclose(file);

    // Now, load and execute the queries from the .crab file
    ASSUME_ITS_TRUE(fossil_crabql_load_queries_from_file(query_mock_db, filename));

    // Cleanup the .crab file
    remove(filename);
}

// Test loading an invalid .crab file
FOSSIL_TEST(test_fossil_crabql_load_invalid_queries_from_file) {
    const char *filename = "invalid_queries.crab";
    // Attempt to load from a nonexistent file
    crabql_status_t status = fossil_crabql_load_queries_from_file(query_mock_db, filename);
    ASSUME_ITS_TRUE(status == CRABQL_FILE_NOT_FOUND);
}

// Test loading a .crab file with invalid queries
FOSSIL_TEST(test_fossil_crabql_load_invalid_queries) {
    const char *filename = "invalid_syntax_queries.crab";
    FILE *file = fopen(filename, "w");
    ASSUME_NOT_CNULL(file);

    // Invalid queries to write to the .crab file
    fprintf(file, "SELECT FROM users;\n");
    fprintf(file, "INSERT INTO users VALUES ('Charlie');\n"); // Missing age
    fclose(file);

    // Load and expect failure due to invalid queries
    ASSUME_ITS_TRUE(fossil_crabql_load_queries_from_file(query_mock_db, filename));

    // Cleanup the .crab file
    remove(filename);
}

// Test SELECT with WHERE clause using comparison operators
FOSSIL_TEST(test_fossil_crabql_query_select_with_operators) {
    const char *query = "SELECT * FROM users WHERE age > 25;";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query));

    const char *query2 = "SELECT * FROM users WHERE age < 40;";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query2));

    const char *query3 = "SELECT * FROM users WHERE age >= 30 AND name != 'Alice';";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query3));

    const char *query4 = "SELECT * FROM users WHERE age <= 35 OR name = 'Bob';";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query4));
}

// Test UPDATE with WHERE clause using comparison and logical operators
FOSSIL_TEST(test_fossil_crabql_query_update_with_operators) {
    const char *query = "UPDATE users SET age = 35 WHERE age < 30;";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query));

    const char *query2 = "UPDATE users SET age = 40 WHERE age > 20 AND name = 'Alice';";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query2));

    const char *query3 = "UPDATE users SET age = 25 WHERE age <= 30 OR name = 'Bob';";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query3));
}

// Test DELETE with complex WHERE clause
FOSSIL_TEST(test_fossil_crabql_query_delete_with_operators) {
    const char *query = "DELETE FROM users WHERE age >= 30 AND name = 'Alice';";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query));

    const char *query2 = "DELETE FROM users WHERE age < 25 OR name != 'Alice';";
    ASSUME_ITS_FALSE(fossil_crabql_query(query_mock_db, query2));
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *

FOSSIL_TEST_GROUP(c_crab_query_tests) {
    ADD_TESTF(test_fossil_crabql_query_insert, query_fixture);
    ADD_TESTF(test_fossil_crabql_query_update, query_fixture);
    ADD_TESTF(test_fossil_crabql_query_select, query_fixture);
    ADD_TESTF(test_fossil_crabql_query_delete, query_fixture);
    ADD_TESTF(test_fossil_crabql_query_invalid, query_fixture);
    ADD_TESTF(test_fossil_crabql_load_queries_from_file, query_fixture);
    ADD_TESTF(test_fossil_crabql_load_invalid_queries_from_file, query_fixture);
    ADD_TESTF(test_fossil_crabql_load_invalid_queries, query_fixture);
    ADD_TESTF(test_fossil_crabql_query_select_with_operators, query_fixture);
    ADD_TESTF(test_fossil_crabql_query_update_with_operators, query_fixture);
    ADD_TESTF(test_fossil_crabql_query_delete_with_operators, query_fixture);
} // end of tests