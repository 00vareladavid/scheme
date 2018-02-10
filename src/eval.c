#include "eval.h"
#include "copy.h"
#include "lval_type.h"
#include "lval_util.h"
#include "gen_util.h"
#include "proc.h"
#include "gc.h"
#include <stdlib.h>
#include <string.h> //TODO remove once you clean up eval_list

/******************************************************************************
* Internal
******************************************************************************/
lval_t* eval_children(sym_env_t* sym_env, lval_t* parent, err_t* err);
lval_t* dispatch_lambda(sym_env_t* sym_map,
                        lval_t* func,
                        lval_t* args,
                        err_t* err);
lval_t* eval_list(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* eval_symbol(sym_env_t* sym_env, lval_t* x, err_t* err);

/* look for symbol, if value found, return a copy
*/
lval_t* eval_symbol(sym_env_t* sym_env, lval_t* x, err_t* err) {
  char* sym = rip_sym(x, err);
  binding_t* binding = sym_search(sym_env, sym);
  free(sym);
  if (!binding) {
    /*
    char* err_msg = malloc(
        (strlen("symbol [] is not bound") + 1 + strlen(sym)) * sizeof(char));
    sprintf(err_msg, "symbol [%s] is not bound", sym);
    free(sym);
    lval_t* a = lval_err(err_msg, err);
    free(err_msg);
    return a;
    */
    return lval_err("symbol unbound", err);
  }

  /* TODO it should actually return a pointer to the value directly */
  return incRef(binding->value);
}

/*
*/
lval_t* eval_children(sym_env_t* sym_env, lval_t* parent, err_t* err) {
  for (lval_t* x = parent; NIL != x; x = x->cdr) {
    x->car = eval_lval(sym_env, x->car, err);
  }

  return parent;
}

/*
*/
lval_t* dispatch_lambda(sym_env_t* sym_map,
                        lval_t* func,
                        lval_t* args,
                        err_t* err) {
  lval_t* params = lval_copy(func->parameters, err);
  lval_t* expression = lval_copy(func->exp, err);
  decRef(func);//TODO is there a way to get rid of this, some kind of interface?

  /* create symbol env */
  // TODO I am just overwriting sym_map here, why do I need to pass it at all?
  sym_map = simple_populate(params, args, err);
  if (err->sig) {
    return lval_clean(expression, NULL);
  }  // couldn't populate

  /* evaluate */
  lval_t* return_val = eval_list(sym_map, expression, err);
  if (err->sig) {
    return NULL;
  }

  /* clean up */
  sym_map_del(sym_map);
  return return_val;
}

/*
*/
lval_t* eval_list(sym_env_t* sym_env, lval_t* args, err_t* err) {
  /* binding constructs and definitions and special procedures? */
  //TODO a lot of fluff here!!
  if (LVAL_SYM == args->car->type) {
    char* first_sym = args->car->identifier;
    if (!strcmp("quote", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return builtin_quote(sym_env, args, err);
    } else if (!strcmp("if", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return builtin_if(sym_env, args, err);
    } else if (!strcmp("lambda", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return builtin_lambda(sym_env, args, err);
    } else if (!strcmp("define", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return builtin_define(sym_env, args, err);
    } else if (!strcmp("set!", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return builtin_set(sym_env, args, err);
    } else if (!strcmp("begin", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return proc_begin(sym_env, args, err);
    }
  }

  /* regular procedures */
  args = eval_children(sym_env, args, err);
  if (LVAL_ERR == args->type) {
    return args;
  }
  lval_t* func = l_pop(&args);

  /* make sure first arg is a func */
  if (LVAL_PROC != func->type) {
    decRef(func);
    decRef(args);
    /* propagate error */
    return lval_err("1st element of list is not a func", err); 
  }

  /* dispatch */
  if (func->builtin) {
    lval_t* x = func->builtin(sym_env, args, err);
    decRef(func);
    return x;
  }

  return dispatch_lambda(sym_env, func, args, err);
}

/******************************************************************************
* External
******************************************************************************/
/* dipsatch eval function based on lval type
*/
lval_t* eval_lval(sym_env_t* sym_env, lval_t* v, err_t* err) {
  switch (v->type) {
    case LVAL_SYM:
      return eval_symbol(sym_env, v, err);
    case LVAL_PAIR:
      return eval_list(sym_env, v, err);
    case LVAL_PROC:
    case LVAL_NUM:
    case LVAL_BOOL:
    case LVAL_UNDEF:
    case LVAL_SPECIAL:
    case LVAL_ERR:
      return v;
    default:
      fucked_up("eval_lval", "I don't recognize this type yo");
      break;
  }
  return NULL; /* should not be reached, just to shut up compiler */
}


