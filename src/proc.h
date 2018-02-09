#pragma once
#include "decl.h"

/*
 */
lval_t* builtin_define(sym_env_t* sym_env, lval_t* args, err_t* err);

/*
 */
lval_t* proc_begin(sym_env_t* sym_env, lval_t* args, err_t* err);

/*
 * - what you want to return is already in a list, just label it a qexp
 */
lval_t* proc_list(sym_env_t* sym_env, lval_t* args, err_t* err);

/*
 */
lval_t* builtin_head(sym_env_t* sym_env, lval_t* args, err_t* err);

/*
 */
lval_t* builtin_tail(sym_env_t* sym_env, lval_t* x, err_t* err);

/*
 */
lval_t* builtin_join(sym_env_t* sym_env, lval_t* args, err_t* err);

/*
 */
lval_t* builtin_eval(sym_env_t* sym_env, lval_t* args, err_t* err);

/* Note checking for overflow should be the job of LISP not here at a low
 * level
 */
lval_t* proc_sum(sym_env_t* sym_env, lval_t* args, err_t* err);

/*
 */
lval_t* proc_minus(sym_env_t* sym_env, lval_t* args, err_t* err);

/*
 */
lval_t* proc_prod(sym_env_t* sym_env, lval_t* args, err_t* err);

/*
 */
lval_t* proc_quotient(sym_env_t* sym_env, lval_t* args, err_t* err);

/*
 */
lval_t* builtin_lambda(sym_env_t* sym_env, lval_t* args, err_t* err);

/*
 */
lval_t* builtin_quote(sym_env_t* sym_map, lval_t* args, err_t* err);

/*
 */
lval_t* builtin_if(sym_env_t* sym_map, lval_t* args, err_t* err);

/* 
 */
lval_t* proc_car(sym_env_t* sym_env, lval_t* args, err_t* err);

/*
 */
lval_t* builtin_set(sym_env_t* sym_env, lval_t* args, err_t* err);
