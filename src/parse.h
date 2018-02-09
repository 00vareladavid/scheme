#pragma once

/* forward decl */
struct lval_t;
typedef struct lval_t lval_t;
struct err_t;
typedef struct err_t err_t;

/* defs */
lval_t* parse_all(char* input, err_t* err); 
lval_t* parse_one(char* input, err_t* err); 
lval_t* read_file(char* filename, err_t* err);
