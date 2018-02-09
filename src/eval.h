#pragma once

/***************************************
* Forward decl
***************************************/
struct lval_t;
typedef struct lval_t lval_t;
struct sym_env_t;
typedef struct sym_env_t sym_env_t;
struct err_t;
typedef struct err_t err_t;

/***************************************
* Defs
***************************************/
lval_t* eval_lval(sym_env_t* sym_env, lval_t* v, err_t* err);
