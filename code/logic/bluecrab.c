/**
 * -----------------------------------------------------------------------------
 * Project: Fossil Logic
 *
 * This file is part of the Fossil Logic project, which aims to develop
 * high-performance, cross-platform applications and libraries. The code
 * contained herein is licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. You may obtain
 * a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * Author: Michael Gene Brockus (Dreamer)
 * Date: 04/05/2014
 *
 * Copyright (C) 2014-2025 Fossil Logic. All rights reserved.
 * -----------------------------------------------------------------------------
 */
#include "fossil/crabdb/bluecrab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* ============================================================================
 * Hashing & Tamper Protection (FNV-1a 64-bit)
 * ============================================================================ */
static uint64_t fnv1a_hash(const void *data, size_t len) {
    const uint8_t *ptr = (const uint8_t*)data;
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i=0;i<len;i++) {
        hash ^= ptr[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

uint64_t fossil_bluecrab_core_hash_record(fossil_bluecrab_core_record_t *record) {
    uint64_t h = 0xcbf29ce484222325ULL; // base
    for (size_t i=0;i<record->value_count;i++) {
        h ^= fnv1a_hash(&record->values[i], sizeof(fossil_bluecrab_core_value_t));
    }
    return h;
}

/* ============================================================================
 * Time utilities
 * ============================================================================ */
time_t fossil_bluecrab_core_now() {
    return time(NULL);
}

/* ============================================================================
 * Database init / free
 * ============================================================================ */
void fossil_bluecrab_core_init(fossil_bluecrab_core_db_t *db){
    if(!db) return;
    db->tables = NULL;
    db->table_count = 0;
    db->in_transaction = false;
}

void fossil_bluecrab_core_free(fossil_bluecrab_core_db_t *db){
    if(!db) return;
    for(size_t t=0;t<db->table_count;t++){
        fossil_bluecrab_core_table_t *table = &db->tables[t];
        for(size_t r=0;r<table->record_count;r++)
            free(table->records[r].values);
        free(table->records);
        free(table->fields);
    }
    free(db->tables);
    db->table_count=0;
}

/* ============================================================================
 * Table CRUD
 * ============================================================================ */
bool fossil_bluecrab_core_create_table(fossil_bluecrab_core_db_t *db,
                                       const char *name,
                                       fossil_bluecrab_core_field_t *fields,
                                       size_t field_count){
    if(!db||!name||!fields||field_count==0) return false;
    db->tables = realloc(db->tables,sizeof(fossil_bluecrab_core_table_t)*(db->table_count+1));
    fossil_bluecrab_core_table_t *table=&db->tables[db->table_count];
    strncpy(table->name,name,63);
    table->fields = malloc(sizeof(fossil_bluecrab_core_field_t)*field_count);
    memcpy(table->fields,fields,sizeof(fossil_bluecrab_core_field_t)*field_count);
    table->field_count=field_count;
    table->records=NULL;
    table->record_count=0;
    db->table_count++;
    return true;
}

bool fossil_bluecrab_core_drop_table(fossil_bluecrab_core_db_t *db,const char *name){
    if(!db||!name) return false;
    for(size_t t=0;t<db->table_count;t++){
        if(strcmp(db->tables[t].name,name)==0){
            fossil_bluecrab_core_table_t *table=&db->tables[t];
            for(size_t r=0;r<table->record_count;r++)
                free(table->records[r].values);
            free(table->records);
            free(table->fields);
            memmove(&db->tables[t],&db->tables[t+1],sizeof(fossil_bluecrab_core_table_t)*(db->table_count-t-1));
            db->table_count--;
            db->tables=realloc(db->tables,sizeof(fossil_bluecrab_core_table_t)*db->table_count);
            return true;
        }
    }
    return false;
}

/* ============================================================================
 * Record CRUD
 * ============================================================================
 */
static fossil_bluecrab_core_table_t* find_table(fossil_bluecrab_core_db_t *db,const char *name){
    if(!db||!name) return NULL;
    for(size_t t=0;t<db->table_count;t++)
        if(strcmp(db->tables[t].name,name)==0) return &db->tables[t];
    return NULL;
}

fossil_bluecrab_core_record_t* fossil_bluecrab_core_insert(fossil_bluecrab_core_db_t *db,
                                                          const char *table_name,
                                                          fossil_bluecrab_core_value_t *values,
                                                          size_t value_count){
    fossil_bluecrab_core_table_t *table=find_table(db,table_name);
    if(!table||value_count!=table->field_count) return NULL;
    table->records=realloc(table->records,sizeof(fossil_bluecrab_core_record_t)*(table->record_count+1));
    fossil_bluecrab_core_record_t *record=&table->records[table->record_count];
    record->id=table->record_count+1;
    record->value_count=value_count;
    record->values=malloc(sizeof(fossil_bluecrab_core_value_t)*value_count);
    memcpy(record->values,values,sizeof(fossil_bluecrab_core_value_t)*value_count);
    record->created_at=record->updated_at=fossil_bluecrab_core_now();
    record->hash=fossil_bluecrab_core_hash_record(record);
    table->record_count++;
    return record;
}

bool fossil_bluecrab_core_update(fossil_bluecrab_core_db_t *db,const char *table_name,size_t record_id,
                                 fossil_bluecrab_core_value_t *values,size_t value_count){
    fossil_bluecrab_core_table_t *table=find_table(db,table_name);
    if(!table||record_id==0||record_id>table->record_count) return false;
    fossil_bluecrab_core_record_t *record=&table->records[record_id-1];
    if(value_count!=record->value_count) return false;
    memcpy(record->values,values,sizeof(fossil_bluecrab_core_value_t)*value_count);
    record->updated_at=fossil_bluecrab_core_now();
    record->hash=fossil_bluecrab_core_hash_record(record);
    return true;
}

bool fossil_bluecrab_core_delete(fossil_bluecrab_core_db_t *db,const char *table_name,size_t record_id){
    fossil_bluecrab_core_table_t *table=find_table(db,table_name);
    if(!table||record_id==0||record_id>table->record_count) return false;
    free(table->records[record_id-1].values);
    memmove(&table->records[record_id-1],&table->records[record_id],
            sizeof(fossil_bluecrab_core_record_t)*(table->record_count-record_id));
    table->record_count--;
    table->records=realloc(table->records,sizeof(fossil_bluecrab_core_record_t)*table->record_count);
    return true;
}

/* ============================================================================
 * Transactions & Git-style logging
 * ============================================================================ */
/* Transaction snapshot structure */
typedef struct fossil_bluecrab_core_tx_snapshot {
    fossil_bluecrab_core_table_t *tables;
    size_t table_count;
} fossil_bluecrab_core_tx_snapshot_t;

/* Global transaction snapshot */
static fossil_bluecrab_core_tx_snapshot_t tx_snapshot = {NULL,0};

/* ============================================================================
 * Helper: Deep copy table
 * ============================================================================ */
static fossil_bluecrab_core_table_t* deep_copy_tables(fossil_bluecrab_core_table_t *tables, size_t table_count) {
    fossil_bluecrab_core_table_t *copy = malloc(sizeof(fossil_bluecrab_core_table_t) * table_count);
    for(size_t t=0; t<table_count; t++) {
        copy[t].field_count = tables[t].field_count;
        copy[t].record_count = tables[t].record_count;
        strncpy(copy[t].name, tables[t].name, 64);
        copy[t].fields = malloc(sizeof(fossil_bluecrab_core_field_t) * tables[t].field_count);
        memcpy(copy[t].fields, tables[t].fields, sizeof(fossil_bluecrab_core_field_t) * tables[t].field_count);
        copy[t].records = malloc(sizeof(fossil_bluecrab_core_record_t) * tables[t].record_count);
        for(size_t r=0; r<tables[t].record_count; r++) {
            copy[t].records[r].id = tables[t].records[r].id;
            copy[t].records[r].value_count = tables[t].records[r].value_count;
            copy[t].records[r].values = malloc(sizeof(fossil_bluecrab_core_value_t) * tables[t].records[r].value_count);
            memcpy(copy[t].records[r].values, tables[t].records[r].values,
                   sizeof(fossil_bluecrab_core_value_t) * tables[t].records[r].value_count);
            copy[t].records[r].created_at = tables[t].records[r].created_at;
            copy[t].records[r].updated_at = tables[t].records[r].updated_at;
            copy[t].records[r].hash = tables[t].records[r].hash;
        }
    }
    return copy;
}

/* ============================================================================
 * Helper: Free deep copied tables
 * ============================================================================ */
static void free_tables_snapshot(fossil_bluecrab_core_table_t *tables, size_t table_count) {
    if(!tables) return;
    for(size_t t=0; t<table_count; t++) {
        for(size_t r=0; r<tables[t].record_count; r++)
            free(tables[t].records[r].values);
        free(tables[t].records);
        free(tables[t].fields);
    }
    free(tables);
}

/* ============================================================================
 * Begin Transaction
 * ============================================================================ */
bool fossil_bluecrab_core_begin_transaction(fossil_bluecrab_core_db_t *db){
    if(!db || db->in_transaction) return false;
    tx_snapshot.tables = deep_copy_tables(db->tables, db->table_count);
    tx_snapshot.table_count = db->table_count;
    db->in_transaction = true;
    return true;
}

/* ============================================================================
 * Commit Transaction (with log + hash)
 * ============================================================================ */
bool fossil_bluecrab_core_commit_transaction(fossil_bluecrab_core_db_t *db){
    if(!db || !db->in_transaction) return false;

    FILE *log = fopen(".bcdlog", "ab");
    if(log) {
        // Timestamp
        time_t ts = time(NULL);
        fwrite(&ts, sizeof(time_t), 1, log);

        // Table count
        fwrite(&db->table_count, sizeof(size_t), 1, log);

        // Write table snapshots
        for(size_t t=0; t<db->table_count; t++) {
            fossil_bluecrab_core_table_t *table = &db->tables[t];
            fwrite(table, sizeof(fossil_bluecrab_core_table_t), 1, log);
        }

        // Simple commit hash: combine table_count + timestamp
        uint64_t commit_hash = ((uint64_t)db->table_count) ^ ((uint64_t)ts);
        fwrite(&commit_hash, sizeof(uint64_t), 1, log);

        fclose(log);
    }

    // Free transaction snapshot
    free_tables_snapshot(tx_snapshot.tables, tx_snapshot.table_count);
    tx_snapshot.tables = NULL;
    tx_snapshot.table_count = 0;

    db->in_transaction = false;
    return true;
}

/* ============================================================================
 * Rollback Transaction
 * ============================================================================ */
bool fossil_bluecrab_core_rollback_transaction(fossil_bluecrab_core_db_t *db){
    if(!db || !db->in_transaction) return false;

    // Free current tables
    for(size_t t=0; t<db->table_count; t++) {
        for(size_t r=0; r<db->tables[t].record_count; r++)
            free(db->tables[t].records[r].values);
        free(db->tables[t].records);
        free(db->tables[t].fields);
    }
    free(db->tables);

    // Restore snapshot
    db->tables = deep_copy_tables(tx_snapshot.tables, tx_snapshot.table_count);
    db->table_count = tx_snapshot.table_count;

    // Free snapshot
    free_tables_snapshot(tx_snapshot.tables, tx_snapshot.table_count);
    tx_snapshot.tables = NULL;
    tx_snapshot.table_count = 0;

    db->in_transaction = false;
    return true;
}

/* ============================================================================
 * Persistence
 * ============================================================================ */
bool fossil_bluecrab_core_save(fossil_bluecrab_core_db_t *db,const char *filename){
    if(!db||!filename) return false;
    FILE *f=fopen(filename,"wb");
    if(!f) return false;
    fwrite(&db->table_count,sizeof(size_t),1,f);
    for(size_t t=0;t<db->table_count;t++){
        fossil_bluecrab_core_table_t *table=&db->tables[t];
        fwrite(&table->field_count,sizeof(size_t),1,f);
        fwrite(table->fields,sizeof(fossil_bluecrab_core_field_t),table->field_count,f);
        fwrite(&table->record_count,sizeof(size_t),1,f);
        for(size_t r=0;r<table->record_count;r++){
            fossil_bluecrab_core_record_t *rec=&table->records[r];
            fwrite(&rec->value_count,sizeof(size_t),1,f);
            fwrite(rec->values,sizeof(fossil_bluecrab_core_value_t),rec->value_count,f);
            fwrite(&rec->created_at,sizeof(time_t),1,f);
            fwrite(&rec->updated_at,sizeof(time_t),1,f);
            fwrite(&rec->hash,sizeof(uint64_t),1,f);
        }
    }
    fclose(f);
    return true;
}

bool fossil_bluecrab_core_load(fossil_bluecrab_core_db_t *db,const char *filename){
    if(!db||!filename) return false;
    fossil_bluecrab_core_free(db);
    FILE *f=fopen(filename,"rb");
    if(!f) return false;
    fread(&db->table_count,sizeof(size_t),1,f);
    db->tables=malloc(sizeof(fossil_bluecrab_core_table_t)*db->table_count);
    for(size_t t=0;t<db->table_count;t++){
        fossil_bluecrab_core_table_t *table=&db->tables[t];
        fread(&table->field_count,sizeof(size_t),1,f);
        table->fields=malloc(sizeof(fossil_bluecrab_core_field_t)*table->field_count);
        fread(table->fields,sizeof(fossil_bluecrab_core_field_t),table->field_count,f);
        fread(&table->record_count,sizeof(size_t),1,f);
        table->records=malloc(sizeof(fossil_bluecrab_core_record_t)*table->record_count);
        for(size_t r=0;r<table->record_count;r++){
            fossil_bluecrab_core_record_t *rec=&table->records[r];
            fread(&rec->value_count,sizeof(size_t),1,f);
            rec->values=malloc(sizeof(fossil_bluecrab_core_value_t)*rec->value_count);
            fread(rec->values,sizeof(fossil_bluecrab_core_value_t),rec->value_count,f);
            fread(&rec->created_at,sizeof(time_t),1,f);
            fread(&rec->updated_at,sizeof(time_t),1,f);
            fread(&rec->hash,sizeof(uint64_t),1,f);
        }
    }
    fclose(f);
    return true;
}

/* ============================================================================
 * Hybrid Query Parser (basic)
 * ============================================================================ */
/* Forward declaration */
static fossil_bluecrab_core_table_t* find_table(fossil_bluecrab_core_db_t *db, const char *name);

/* ============================================================================
 * Lexer & Tokenizer
 * ============================================================================ */
typedef enum {
    TOK_FIELD, TOK_VALUE, TOK_OP, TOK_AND, TOK_OR, TOK_LPAREN, TOK_RPAREN, TOK_END
} token_type_t;

typedef struct {
    token_type_t type;
    char text[256];
} token_t;

typedef struct {
    const char *str;
    size_t pos;
} lexer_t;

static void lexer_skip_space(lexer_t *lex) {
    while(isspace(lex->str[lex->pos])) lex->pos++;
}

static token_t lexer_next(lexer_t *lex) {
    lexer_skip_space(lex);
    token_t tok = {TOK_END,""};
    char c = lex->str[lex->pos];
    if(c==0) { tok.type=TOK_END; return tok; }
    if(c=='(') { tok.type=TOK_LPAREN; lex->pos++; return tok; }
    if(c==')') { tok.type=TOK_RPAREN; lex->pos++; return tok; }
    if(strncmp(&lex->str[lex->pos],"AND",3)==0 && !isalnum(lex->str[lex->pos+3])) { lex->pos+=3; tok.type=TOK_AND; return tok; }
    if(strncmp(&lex->str[lex->pos],"OR",2)==0 && !isalnum(lex->str[lex->pos+2])) { lex->pos+=2; tok.type=TOK_OR; return tok; }

    // Operators
    if(strncmp(&lex->str[lex->pos],"==",2)==0) { lex->pos+=2; tok.type=TOK_OP; strcpy(tok.text,"=="); return tok; }
    if(strncmp(&lex->str[lex->pos],"!=",2)==0) { lex->pos+=2; tok.type=TOK_OP; strcpy(tok.text,"!="); return tok; }
    if(strncmp(&lex->str[lex->pos],">=",2)==0) { lex->pos+=2; tok.type=TOK_OP; strcpy(tok.text,">="); return tok; }
    if(strncmp(&lex->str[lex->pos],"<=",2)==0) { lex->pos+=2; tok.type=TOK_OP; strcpy(tok.text,"<="); return tok; }
    if(c=='>' || c=='<') { lex->pos++; tok.type=TOK_OP; tok.text[0]=c; tok.text[1]=0; return tok; }

    // Field or value
    size_t start=lex->pos;
    if(c=='"'){ // quoted string
        lex->pos++;
        start=lex->pos;
        while(lex->str[lex->pos] && lex->str[lex->pos]!='"') lex->pos++;
        strncpy(tok.text,&lex->str[start],lex->pos-start);
        tok.text[lex->pos-start]=0;
        lex->pos++; // skip closing quote
        tok.type=TOK_VALUE;
        return tok;
    }

    while(lex->str[lex->pos] && !isspace(lex->str[lex->pos]) && strchr("()><=!&|",lex->str[lex->pos])==NULL) lex->pos++;
    strncpy(tok.text,&lex->str[start],lex->pos-start);
    tok.text[lex->pos-start]=0;
    // Decide if value or field
    tok.type = isdigit(tok.text[0]) || tok.text[0]=='-' ? TOK_VALUE : TOK_FIELD;
    return tok;
}

/* ============================================================================
 * Expression Node
 * ============================================================================ */
typedef enum { EXPR_COND, EXPR_AND, EXPR_OR } expr_type_t;

typedef struct expr_node {
    expr_type_t type;
    char field[64];
    char op[3];
    char value[256];
    struct expr_node *left;
    struct expr_node *right;
} expr_node_t;

/* ============================================================================
 * Parser
 * ============================================================================ */
static expr_node_t* parse_expr(lexer_t *lex);

static expr_node_t* parse_primary(lexer_t *lex) {
    token_t tok = lexer_next(lex);
    if (tok.type==TOK_LPAREN){
        expr_node_t *node = parse_expr(lex);
        token_t next = lexer_next(lex);
        if (next.type!=TOK_RPAREN){ free(node); return NULL; }
        return node;
    }
    if(tok.type!=TOK_FIELD) return NULL;
    char field[64]; strcpy(field,tok.text);
    token_t op = lexer_next(lex);
    if(op.type!=TOK_OP) return NULL;
    token_t val = lexer_next(lex);
    if(val.type!=TOK_VALUE && val.type!=TOK_FIELD) return NULL;

    expr_node_t *node = malloc(sizeof(expr_node_t));
    node->type = EXPR_COND;
    strcpy(node->field, field);
    strcpy(node->op, op.text);
    strcpy(node->value, val.text);
    node->left=node->right=NULL;
    return node;
}

static expr_node_t* parse_expr(lexer_t *lex) {
    expr_node_t *left = parse_primary(lex);
    if(!left) return NULL;

    while (1){
        lexer_skip_space(lex);
        size_t pos=lex->pos;
        token_t tok = lexer_next(lex);
        if(tok.type==TOK_AND || tok.type==TOK_OR){
            expr_node_t *node = malloc(sizeof(expr_node_t));
            node->type = tok.type==TOK_AND?EXPR_AND:EXPR_OR;
            node->left = left;
            node->right = parse_primary(lex);
            left = node;
        } else {
            lex->pos = pos;
            break;
        }
    }
    return left;
}

/* ============================================================================
 * Evaluator
 * ============================================================================ */
static bool eval_condition(fossil_bluecrab_core_record_t *rec, fossil_bluecrab_core_table_t *table,
                           const char *field, const char *op, const char *value_str) {
    for(size_t f=0; f<table->field_count; f++){
        if(strcmp(table->fields[f].name, field)!=0) continue;
        fossil_bluecrab_core_value_t *v = &rec->values[f];

        switch(v->type){
            case FBC_TYPE_I8:
            case FBC_TYPE_I16:
            case FBC_TYPE_I32:
            case FBC_TYPE_I64:{
                long long val = atoll(value_str);
                long long vval = (v->type==FBC_TYPE_I8?v->value.i8:
                                  v->type==FBC_TYPE_I16?v->value.i16:
                                  v->type==FBC_TYPE_I32?v->value.i32:v->value.i64);
                if(strcmp(op,"==")==0) return vval==val;
                if(strcmp(op,"!=")==0) return vval!=val;
                if(strcmp(op,">")==0) return vval>val;
                if(strcmp(op,"<")==0) return vval<val;
                if(strcmp(op,">=")==0) return vval>=val;
                if(strcmp(op,"<=")==0) return vval<=val;
                break;
            }
            case FBC_TYPE_F32:
            case FBC_TYPE_F64:{
                double val = atof(value_str);
                double vval = (v->type==FBC_TYPE_F32?v->value.f32:v->value.f64);
                if(strcmp(op,"==")==0) return vval==val;
                if(strcmp(op,"!=")==0) return vval!=val;
                if(strcmp(op,">")==0) return vval>val;
                if(strcmp(op,"<")==0) return vval<val;
                if(strcmp(op,">=")==0) return vval>=val;
                if(strcmp(op,"<=")==0) return vval<=val;
                break;
            }
            case FBC_TYPE_CSTR:{
                if(strcmp(op,"==")==0) return strcmp(v->value.cstr,value_str)==0;
                if(strcmp(op,"!=")==0) return strcmp(v->value.cstr,value_str)!=0;
                break;
            }
            case FBC_TYPE_BOOL:{
                bool val = (strcasecmp(value_str,"true")==0)?true:false;
                if(strcmp(op,"==")==0) return v->value.b==val;
                if(strcmp(op,"!=")==0) return v->value.b!=val;
                break;
            }
            default: break;
        }
    }
    return false;
}

static bool eval_expr(fossil_bluecrab_core_record_t *rec, fossil_bluecrab_core_table_t *table, expr_node_t *node) {
    if(!node) return false;
    switch(node->type){
        case EXPR_COND: return eval_condition(rec,table,node->field,node->op,node->value);
        case EXPR_AND: return eval_expr(rec,table,node->left) && eval_expr(rec,table,node->right);
        case EXPR_OR: return eval_expr(rec,table,node->left) || eval_expr(rec,table,node->right);
    }
    return false;
}

static void free_expr(expr_node_t *node){
    if(!node) return;
    free_expr(node->left);
    free_expr(node->right);
    free(node);
}

/* ============================================================================
 * Query
 * ============================================================================ */
fossil_bluecrab_core_record_t* fossil_bluecrab_core_query(fossil_bluecrab_core_db_t *db,
                                                          const char *table_name,
                                                          const char *query){
    fossil_bluecrab_core_table_t *table = find_table(db, table_name);
    if(!table || !query) return NULL;

    lexer_t lex = {query, 0};
    expr_node_t *expr = parse_expr(&lex);
    if(!expr) return NULL;

    for(size_t r=0; r<table->record_count; r++){
        if(eval_expr(&table->records[r], table, expr)){
            free_expr(expr);
            return &table->records[r];
        }
    }
    free_expr(expr);
    return NULL;
}

/* ============================================================================
 * Print Utilities
 * ============================================================================ */
void fossil_bluecrab_core_print_record(fossil_bluecrab_core_record_t *record){
    if(!record) return;
    printf("Record ID:%zu | Created:%ld Updated:%ld | Hash:%llu\n",record->id,record->created_at,record->updated_at,(unsigned long long)record->hash);
    for(size_t i=0;i<record->value_count;i++){
        fossil_bluecrab_core_value_t *v=&record->values[i];
        switch(v->type){
            case FBC_TYPE_I8: printf("i8=%d ",v->value.i8); break;
            case FBC_TYPE_I16: printf("i16=%d ",v->value.i16); break;
            case FBC_TYPE_I32: printf("i32=%d ",v->value.i32); break;
            case FBC_TYPE_I64: printf("i64=%lld ",(long long)v->value.i64); break;
            case FBC_TYPE_U8: printf("u8=%u ",v->value.u8); break;
            case FBC_TYPE_U16: printf("u16=%u ",v->value.u16); break;
            case FBC_TYPE_U32: printf("u32=%u ",v->value.u32); break;
            case FBC_TYPE_U64: printf("u64=%llu ",(unsigned long long)v->value.u64); break;
            case FBC_TYPE_F32: printf("f32=%f ",v->value.f32); break;
            case FBC_TYPE_F64: printf("f64=%f ",v->value.f64); break;
            case FBC_TYPE_CSTR: printf("cstr=%s ",v->value.cstr); break;
            case FBC_TYPE_CHAR: printf("char=%c ",v->value.c); break;
            case FBC_TYPE_BOOL: printf("bool=%d ",v->value.b); break;
            default: printf("type=%d ",v->type); break;
        }
    }
    printf("\n");
}

void fossil_bluecrab_core_print_table(fossil_bluecrab_core_table_t *table){
    if(!table) return;
    printf("Table: %s | Records: %zu\n",table->name,table->record_count);
    for(size_t r=0;r<table->record_count;r++)
        fossil_bluecrab_core_print_record(&table->records[r]);
}
