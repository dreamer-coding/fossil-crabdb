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
#include "fossil/crabdb/myshell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ===========================================================
// Helper functions
// ===========================================================

static FILE* open_file(const char *file_name, const char *mode) {
    return fopen(file_name, mode);
}

static void close_file(FILE *file) {
    if (file != NULL) {
        fclose(file);
    }
}

// ===========================================================
// CRUD Operations
// ===========================================================

fossil_myshell_error_t fossil_myshell_create_record(const char *file_name, const char *key, const char *value) {
    FILE *file = open_file(file_name, "a");
    if (!file) {
        return FOSSIL_MYSHELL_ERROR_IO;
    }

    fprintf(file, "%s=%s\n", key, value);
    close_file(file);

    return FOSSIL_MYSHELL_ERROR_SUCCESS;
}

fossil_myshell_error_t fossil_myshell_read_record(const char *file_name, const char *key, char *value, size_t buffer_size) {
    FILE *file = open_file(file_name, "r");
    if (!file) {
        return FOSSIL_MYSHELL_ERROR_IO;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *line_key = strtok(line, "=");
        char *line_value = strtok(NULL, "\n");

        if (line_key && strcmp(line_key, key) == 0) {
            strncpy(value, line_value, buffer_size);
            close_file(file);
            return FOSSIL_MYSHELL_ERROR_SUCCESS;
        }
    }

    close_file(file);
    return FOSSIL_MYSHELL_ERROR_NOT_FOUND;
}

fossil_myshell_error_t fossil_myshell_update_record(const char *file_name, const char *key, const char *new_value) {
    FILE *file = open_file(file_name, "r+");
    if (!file) {
        return FOSSIL_MYSHELL_ERROR_IO;
    }

    char line[256];
    long pos;
    bool found = false;
    while ((pos = ftell(file)) != -1 && fgets(line, sizeof(line), file)) {
        char *line_key = strtok(line, "=");

        if (line_key && strcmp(line_key, key) == 0) {
            found = true;
            break;
        }
    }

    if (!found) {
        close_file(file);
        return FOSSIL_MYSHELL_ERROR_NOT_FOUND;
    }

    fseek(file, pos, SEEK_SET);
    fprintf(file, "%s=%s\n", key, new_value);
    close_file(file);

    return FOSSIL_MYSHELL_ERROR_SUCCESS;
}

fossil_myshell_error_t fossil_myshell_delete_record(const char *file_name, const char *key) {
    FILE *file = open_file(file_name, "r");
    if (!file) {
        return FOSSIL_MYSHELL_ERROR_IO;
    }

    FILE *temp_file = open_file("temp.crabdb", "w");
    if (!temp_file) {
        close_file(file);
        return FOSSIL_MYSHELL_ERROR_IO;
    }

    char line[256];
    bool deleted = false;
    while (fgets(line, sizeof(line), file)) {
        char *line_key = strtok(line, "=");
        if (line_key && strcmp(line_key, key) == 0) {
            deleted = true;
        } else {
            fputs(line, temp_file);
        }
    }

    close_file(file);
    close_file(temp_file);

    if (deleted) {
        remove(file_name);
        rename("temp.crabdb", file_name);
        return FOSSIL_MYSHELL_ERROR_SUCCESS;
    }

    remove("temp.crabdb");
    return FOSSIL_MYSHELL_ERROR_NOT_FOUND;
}

// ===========================================================
// Database Management
// ===========================================================

fossil_myshell_error_t fossil_myshell_create_database(const char *file_name) {
    FILE *file = open_file(file_name, "w");
    if (!file) {
        return FOSSIL_MYSHELL_ERROR_IO;
    }

    close_file(file);
    return FOSSIL_MYSHELL_ERROR_SUCCESS;
}

fossil_myshell_error_t fossil_myshell_open_database(const char *file_name) {
    FILE *file = open_file(file_name, "r");
    if (!file) {
        return FOSSIL_MYSHELL_ERROR_FILE_NOT_FOUND;
    }

    close_file(file);
    return FOSSIL_MYSHELL_ERROR_SUCCESS;
}

fossil_myshell_error_t fossil_myshell_close_database(const char *file_name) {
    return FOSSIL_MYSHELL_ERROR_SUCCESS;  // No specific action required for closure.
}

fossil_myshell_error_t fossil_myshell_delete_database(const char *file_name) {
    if (remove(file_name) == 0) {
        return FOSSIL_MYSHELL_ERROR_SUCCESS;
    }

    return FOSSIL_MYSHELL_ERROR_IO;
}

// ===========================================================
// Backup and Restore
// ===========================================================

fossil_myshell_error_t fossil_myshell_backup_database(const char *source_file, const char *backup_file) {
    FILE *source = open_file(source_file, "rb");
    if (!source) {
        return FOSSIL_MYSHELL_ERROR_IO;
    }

    FILE *backup = open_file(backup_file, "wb");
    if (!backup) {
        close_file(source);
        return FOSSIL_MYSHELL_ERROR_IO;
    }

    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytes_read, backup);
    }

    close_file(source);
    close_file(backup);

    return FOSSIL_MYSHELL_ERROR_SUCCESS;
}

fossil_myshell_error_t fossil_myshell_restore_database(const char *backup_file, const char *destination_file) {
    FILE *backup = open_file(backup_file, "rb");
    if (!backup) {
        return FOSSIL_MYSHELL_ERROR_IO;
    }

    FILE *destination = open_file(destination_file, "wb");
    if (!destination) {
        close_file(backup);
        return FOSSIL_MYSHELL_ERROR_IO;
    }

    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), backup)) > 0) {
        fwrite(buffer, 1, bytes_read, destination);
    }

    close_file(backup);
    close_file(destination);

    return FOSSIL_MYSHELL_ERROR_SUCCESS;
}

// ===========================================================
// Query and Data Validation
// ===========================================================

bool fossil_myshell_validate_extension(const char *file_name) {
    return strstr(file_name, ".crabdb") != NULL;
}

bool fossil_myshell_validate_data(const char *data) {
    return data != NULL && strlen(data) > 0;
}
