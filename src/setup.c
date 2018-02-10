#include "setup.h"
#include "lval_type.h"
#include "gen_util.h"
#include "sym_env.h"
#include "proc.h"
#include "predicate.h"
#include "features.h"
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
* GLOBAL
*******************************************************************************/
char* builtin_names[] = {"begin",    "eval",    "list",    "car",
                         "boolean?", "number?", "symbol?", "procedure?",
                         "pair?",    "eqv?",    "+",       "-",
                         "*",        "/",       "quote",   NULL};

/*******************************************************************************
* Internal
*******************************************************************************/
/* dispatch builtin function based on string
 */
builtin_fun* proc_tab(char* x) {
  if (!strcmp("define", x)) {
    return builtin_define;
  } else if (!strcmp("quote", x)) {
    return builtin_quote;
  } else if (!strcmp("begin", x)) {
    return proc_begin;
  } else if (!strcmp("boolean?", x)) {
    return proc_pred_bool;
  } else if (!strcmp("number?", x)) {
    return proc_pred_num;
  } else if (!strcmp("symbol?", x)) {
    return proc_pred_sym;
  } else if (!strcmp("procedure?", x)) {
    return proc_pred_fun;
  } else if (!strcmp("pair?", x)) {
    return proc_pred_pair;
  } else if (!strcmp("eqv?", x)) {
    return proc_pred_eqv;
  } else if (!strcmp("car", x)) {
    return proc_car;
  } else if (!strcmp("lambda", x)) {
    return builtin_lambda;
  } else if (!strcmp("eval", x)) {
    return builtin_eval;
  } else if (!strcmp("list", x)) {
    return proc_list;
  } else if (!strcmp("+", x)) {
    return proc_sum;
  } else if (!strcmp("-", x)) {
    return proc_minus;
  } else if (!strcmp("*", x)) {
    return proc_prod;
  } else if (!strcmp("/", x)) {
    return proc_quotient;
  }

  fucked_up("proc_tab", "unrecognized string");
  return 0;
}

/* NOTE: if any errors occur, no need to free manually
 * - this is because an error here is a critical error and will force the 
 *   program to crash
 */
void push_builtin(sym_env_t* sym_map, char* fun_name, err_t* err) {
  char* funx = strdup_e(fun_name, err);
  if (err->sig) {
    return;
  }

  lval_t* xx = lval_builtin(proc_tab(fun_name), err);
  if (err->sig) {
    return;
  }

  symbol_add(sym_map, funx, xx, err);  /* PPG ERR */
}

/*******************************************************************************
* External
*******************************************************************************/
/*
 */
err_t* set_base_err(void) {
  err_t* x = malloc(sizeof(err_t));
  if (!x) {
    fucked_up("set_base_err", "1 insufficient mem for baseline framework");
  }
  x->sig = OK;
  return x;
}

/*
 */
void init_global_map(err_t* err) {
  sym_env_t* sym_map = sym_map_make(err);
  if (err->sig) {
    fucked_up("init_global_map","2 insufficient mem for baseline framework");
  }

  char* x;
  for (size_t i = 0; (x = builtin_names[i]); i = inc_size(i)) {
    push_builtin(sym_map, x, err);
    if (err->sig) {
      fucked_up("init_global_map","2 insufficient mem for baseline framework");
    }
  }

  symbol_add(sym_map, strdup_e("#t", err), LISP_TRUE, err);
  symbol_add(sym_map, strdup_e("#f", err), LISP_FALSE, err);

  if (err->sig) {
    fucked_up("init_global_map","2 insufficient mem for baseline framework");
  }
  set_global_map(sym_map);
}

/*
 */
void load_stdlib(sym_env_t* sym_env, err_t* err) {
  exec_file(sym_env, "lisp/std_defs.lisp", err);
}
