#pragma once
#include "decl.h"

/*
 */
lval_t* proc_pred_bool(sym_env_t* sym_map, lval_t* args, err_t* err);

/*
 */
lval_t* proc_pred_num(sym_env_t* sym_map, lval_t* args, err_t* err);

/*
 */
lval_t* proc_pred_sym(sym_env_t* sym_map, lval_t* args, err_t* err);

/*
 */
lval_t* proc_pred_fun(sym_env_t* sym_map, lval_t* args, err_t* err);

/*
 */
lval_t* proc_pred_pair(sym_env_t* sym_map, lval_t* args, err_t* err);

/*
 */
lval_t* proc_pred_eqv(sym_env_t* sym_map, lval_t* args, err_t* err);
