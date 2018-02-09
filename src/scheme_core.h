#pragma once

/* forward decl */
struct lval_t;
typedef struct lval_t lval_t;
struct err_t;
typedef struct err_t err_t;
struct sym_env_t;
typedef struct sym_env_t sym_env_t;

/* defs */
lval_t* lisp_exec(sym_env_t* sym_env, char* input, err_t* err);
void exec_file(sym_env_t* sym_env, char* filename, err_t* err); 
