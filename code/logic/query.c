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
#include "fossil/crabdb/query.h"
#include <ctype.h>

// -----------------------------------------------------------------------------
// Context Structure
// -----------------------------------------------------------------------------

struct crabql_context {
    char* active_backend;   /* name of the backend currently loaded */
    void* backend_handle;   /* opaque pointer to backend instance */
    bool  opened;           /* true if a db is open */
};

/* --------------------------------------------------------------------------
 * Tiny symbol table for assignments
 * -------------------------------------------------------------------------- */
typedef struct symbol {
    char* name;
    char* value;   /* store as stringified JSON/text */
    struct symbol* next;
} symbol_t;

static symbol_t* g_symbols = NULL;

/* --------------------------------------------------------------------------
 * Simple CrabQL Script Executor
 * --------------------------------------------------------------------------
 * Syntax (inspired by Meson/Python):
 *
 *   # comments start with #
 *   foo = 42
 *   insert("key", {"val": foo})
 *   if foo > 10:
 *       update("key", {"new": foo})
 * -------------------------------------------------------------------------- */

static const char* skip_ws_and_comments(const char* p) {
    while (*p) {
        if (*p == '#') {
            while (*p && *p != '\n') p++;
        } else if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
            p++;
        } else {
            break;
        }
    }
    return p;
}

static bool parse_identifier(const char** p, char* out, size_t outsz) {
    const char* s = *p;
    size_t i = 0;
    if (!isalpha((unsigned char)*s) && *s != '_') return false;
    while (isalnum((unsigned char)*s) || *s == '_') {
        if (i + 1 < outsz) out[i++] = *s;
        s++;
    }
    out[i] = '\0';
    *p = s;
    return true;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static void crabql_free_backend(crabql_context_t* ctx) {
    if (!ctx) return;
    if (ctx->active_backend) {
        free(ctx->active_backend);
        ctx->active_backend = NULL;
    }
    ctx->backend_handle = NULL;
    ctx->opened = false;
}

// -----------------------------------------------------------------------------
// Context Management
// -----------------------------------------------------------------------------

crabql_context_t* fossil_bluecrab_query_create(void) {
    crabql_context_t* ctx = (crabql_context_t*)calloc(1, sizeof(crabql_context_t));
    return ctx;
}

void fossil_bluecrab_query_destroy(crabql_context_t* ctx) {
    if (!ctx) return;
    crabql_free_backend(ctx);
    free(ctx);
}

static void symbol_set(const char* name, const char* value) {
    for (symbol_t* s = g_symbols; s; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            free(s->value);
            s->value = strdup(value);
            return;
        }
    }
    symbol_t* s = calloc(1, sizeof(*s));
    s->name = strdup(name);
    s->value = strdup(value);
    s->next = g_symbols;
    g_symbols = s;
}

static const char* symbol_get(const char* name) {
    for (symbol_t* s = g_symbols; s; s = s->next) {
        if (strcmp(s->name, name) == 0) return s->value;
    }
    return NULL;
}

/* --------------------------------------------------------------------------
 * Lexer helpers
 * -------------------------------------------------------------------------- */
static const char* skip_ws_and_comments(const char* p) {
    while (*p) {
        if (*p == '#') {
            while (*p && *p != '\n') p++;
        } else if (isspace((unsigned char)*p)) {
            p++;
        } else break;
    }
    return p;
}

static bool parse_identifier(const char** p, char* out, size_t outsz) {
    const char* s = *p;
    size_t i = 0;
    if (!isalpha((unsigned char)*s) && *s != '_') return false;
    while (isalnum((unsigned char)*s) || *s == '_') {
        if (i + 1 < outsz) out[i++] = *s;
        s++;
    }
    out[i] = '\0';
    *p = s;
    return true;
}

/* --------------------------------------------------------------------------
 * Expression parsing
 * -------------------------------------------------------------------------- */

static char* parse_string(const char** p) {
    const char* s = *p;
    if (*s != '"') return NULL;
    s++;
    char buf[256];
    size_t i = 0;
    while (*s && *s != '"') {
        if (*s == '\\' && s[1]) {
            s++;
        }
        if (i + 1 < sizeof(buf)) buf[i++] = *s;
        s++;
    }
    if (*s != '"') return NULL;
    buf[i] = '\0';
    *p = s + 1;
    return strdup(buf);
}

static char* parse_number(const char** p) {
    const char* s = *p;
    char buf[64];
    size_t i = 0;
    if (*s == '-' || *s == '+') buf[i++] = *s++;
    while (isdigit((unsigned char)*s) || *s == '.') {
        if (i + 1 < sizeof(buf)) buf[i++] = *s;
        s++;
    }
    buf[i] = '\0';
    *p = s;
    return strdup(buf);
}

static char* parse_object(const char** p);

static char* parse_expression(const char** p) {
    *p = skip_ws_and_comments(*p);
    if (**p == '"') {
        return parse_string(p);
    } else if (isdigit((unsigned char)**p) || **p == '-' || **p == '+') {
        return parse_number(p);
    } else if (**p == '{') {
        return parse_object(p);
    } else {
        /* identifier (lookup from symbols) */
        char ident[64];
        if (parse_identifier(p, ident, sizeof(ident))) {
            const char* val = symbol_get(ident);
            if (val) return strdup(val);
            return strdup(ident);
        }
    }
    return NULL;
}

static char* parse_object(const char** p) {
    const char* s = *p;
    if (*s != '{') return NULL;
    s++;
    char buf[512];
    size_t i = 0;
    buf[i++] = '{';

    while (1) {
        s = skip_ws_and_comments(s);
        if (*s == '}') { buf[i++] = '}'; s++; break; }
        char* key = parse_string(&s);
        if (!key) return NULL;
        i += snprintf(buf + i, sizeof(buf) - i, "\"%s\":", key);
        free(key);

        s = skip_ws_and_comments(s);
        if (*s != ':') return NULL;
        s++;
        char* val = parse_expression(&s);
        if (!val) return NULL;
        i += snprintf(buf + i, sizeof(buf) - i, "\"%s\"", val);
        free(val);

        s = skip_ws_and_comments(s);
        if (*s == ',') { buf[i++] = ','; s++; continue; }
        else if (*s == '}') { buf[i++] = '}'; s++; break; }
        else return NULL;
    }

    buf[i] = '\0';
    *p = s;
    return strdup(buf);
}

/* --------------------------------------------------------------------------
 * Function call parsing
 * -------------------------------------------------------------------------- */
static void parse_arguments(const char** p) {
    if (**p != '(') return;
    (*p)++;
    printf("  args: ");
    while (1) {
        *p = skip_ws_and_comments(*p);
        if (**p == ')') { (*p)++; break; }
        char* arg = parse_expression(p);
        if (arg) {
            printf("%s ", arg);
            free(arg);
        }
        *p = skip_ws_and_comments(*p);
        if (**p == ',') { (*p)++; continue; }
        else if (**p == ')') { (*p)++; break; }
        else break;
    }
    printf("\n");
}

/* --------------------------------------------------------------------------
 * Main executor
 * -------------------------------------------------------------------------- */
bool fossil_bluecrab_query_exec(crabql_context_t* ctx, const char* code) {
    if (!ctx || !code) return false;
    const char* p = code;

    while (*p) {
        p = skip_ws_and_comments(p);
        if (!*p) break;

        char ident[64];
        if (!parse_identifier(&p, ident, sizeof(ident))) {
            fprintf(stderr, "[CrabQL] Syntax error near: %s\n", p);
            return false;
        }

        p = skip_ws_and_comments(p);

        if (*p == '=') {
            /* assignment */
            p++;
            char* expr = parse_expression(&p);
            if (!expr) return false;
            symbol_set(ident, expr);
            printf("[CrabQL] Assignment: %s = %s\n", ident, expr);
            free(expr);
        } else if (*p == '(') {
            /* function call */
            printf("[CrabQL] Function call: %s\n", ident);
            parse_arguments(&p);
        } else if (*p == ':') {
            /* block header (not implemented yet) */
            printf("[CrabQL] Block start: %s\n", ident);
            p++;
        } else {
            fprintf(stderr, "[CrabQL] Unexpected token after %s\n", ident);
            return false;
        }

        /* advance to next line */
        while (*p && *p != '\n') p++;
    }

    return true;
}

bool fossil_bluecrab_query_exec_file(crabql_context_t* ctx, const char* path) {
    if (!ctx || !path) return false;

    FILE* f = fopen(path, "rb");  // use binary mode to avoid newline issues
    if (!f) {
        fprintf(stderr, "[CrabQL] Failed to open file: %s\n", path);
        return false;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return false;
    }

    long len = ftell(f);
    if (len < 0) {
        fclose(f);
        return false;
    }
    rewind(f);

    char* buf = (char*)malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return false;
    }

    size_t nread = fread(buf, 1, (size_t)len, f);
    buf[nread] = '\0';  // always terminate
    fclose(f);

    if (nread < (size_t)len) {
        fprintf(stderr, "[CrabQL] Warning: expected %ld bytes, read %zu\n", len, nread);
    }

    bool ok = fossil_bluecrab_query_exec(ctx, buf);
    free(buf);
    return ok;
}

// -----------------------------------------------------------------------------
// Database Utilities (generic stubs)
// -----------------------------------------------------------------------------

bool fossil_bluecrab_query_open(crabql_context_t* ctx, const char* dbfile) {
    if (!ctx || !dbfile) return false;
    if (ctx->opened) fossil_bluecrab_query_close(ctx);
    printf("[CrabQL] Opening DB file: %s\n", dbfile);
    ctx->opened = true;
    return true;
}

bool fossil_bluecrab_query_close(crabql_context_t* ctx) {
    if (!ctx || !ctx->opened) return false;
    printf("[CrabQL] Closing DB\n");
    ctx->opened = false;
    return true;
}

bool fossil_bluecrab_query_insert(crabql_context_t* ctx,
                                  const char* key,
                                  const char* json_value) {
    if (!ctx || !ctx->opened || !key) return false;
    printf("[CrabQL] Insert key='%s', value=%s\n", key, json_value);
    return true;
}

bool fossil_bluecrab_query_update(crabql_context_t* ctx,
                                  const char* key,
                                  const char* json_value) {
    if (!ctx || !ctx->opened || !key) return false;
    printf("[CrabQL] Update key='%s', value=%s\n", key, json_value);
    return true;
}

bool fossil_bluecrab_query_remove(crabql_context_t* ctx, const char* key) {
    if (!ctx || !ctx->opened || !key) return false;
    printf("[CrabQL] Remove key='%s'\n", key);
    return true;
}

char* fossil_bluecrab_query_get(crabql_context_t* ctx, const char* key) {
    if (!ctx || !ctx->opened || !key) {
        fprintf(stderr, "[CrabQL] Invalid get() call\n");
        return NULL;
    }

    printf("[CrabQL] Get key='%s'\n", key);

    // TODO: choose backend dynamically
    // For now, fake a JSON response with key interpolation
    size_t len = strlen(key) + 32;
    char* result = malloc(len);
    if (!result) return NULL;

    snprintf(result, len, "{\"%s\":\"mock_value\"}", key);
    return result;
}

size_t fossil_bluecrab_query_count(crabql_context_t* ctx) {
    if (!ctx || !ctx->opened) return 0;
    printf("[CrabQL] Count entries\n");
    return 42; /* stubbed */
}

// -----------------------------------------------------------------------------
// Module Import API
// -----------------------------------------------------------------------------

bool fossil_bluecrab_query_import(crabql_context_t* ctx, const char* module) {
    if (!ctx || !module) return false;

    // Free any previously loaded backend
    crabql_free_backend(ctx);

    // Decide which backend to attach
    if (strcmp(module, "fileshell") == 0) {
        ctx->backend_type = CRABQL_BACKEND_FILE;
        ctx->backend = fossil_fileshell_create();
    } else if (strcmp(module, "cacheshell") == 0) {
        ctx->backend_type = CRABQL_BACKEND_CACHE;
        ctx->backend = fossil_cacheshell_create();
    } else if (strcmp(module, "timeshell") == 0) {
        ctx->backend_type = CRABQL_BACKEND_TIME;
        ctx->backend = fossil_timeshell_create();
    } else if (strcmp(module, "myshell") == 0) {
        ctx->backend_type = CRABQL_BACKEND_MY;
        ctx->backend = fossil_myshell_create();
    } else if (strcmp(module, "noshell") == 0) {
        ctx->backend_type = CRABQL_BACKEND_NO;
        ctx->backend = fossil_noshell_create();
    } else {
        fprintf(stderr, "[CrabQL] Unknown module: %s\n", module);
        return false;
    }

    ctx->active_backend = strdup(module);
    printf("[CrabQL] Imported module: %s\n", module);
    return true;
}
