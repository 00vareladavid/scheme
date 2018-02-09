#include "proc.h"
#include "gen_util.h"
#include "lval_type.h"
#include "lval_util.h"
#include "proc_util.h"
#include "eval.h"
/*******************************************************************************
* Internal
*******************************************************************************/

/*******************************************************************************
* External
*******************************************************************************/
/*
 */
lval_t* builtin_define(sym_env_t* sym_env, lval_t* args, err_t* err) {
  /* Process arguments */
  lval_t *one, *x;
  if( extract_arg(args,2,&one,&x) ) {
    return lval_err("DEFINE : incorrect number of arguments", err);
  }

  if (!scheme_assert_type(LVAL_SYM, one)) {
    return lval_err("DEFINE : first arg should be a symbol", err);
  }
  char* symbol = rip_sym(one, err);

  /* eval */
  x = eval_lval(sym_env, x, err);
  if (err->sig) {
    free(symbol);
    return lval_clean(x, NULL);
  }

  /* push */
  /* note: adding directly to global SYM_MAP */
  if (global_symbol_add(symbol, x, err)) {
    return lval_err("DEFINE : symbol already bound", err);
  } else if (err->sig) { /* werent able to make space in the sym_map */
    fucked_up("builtin_define", "TODO global symbol stack error");
  }

  return lval_undef(err);
}

/*
 */
lval_t* proc_begin(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);
  lval_t* r = NIL;
  while (x) {
    decRef(r);  // TODO is this right?
    r = eval_lval(sym_env, x, err);
    x = l_pop(&args);
  }

  decRef(args);
  return r;
}

/*
 * - what you want to return is already in a list, just label it a qexp
 */
lval_t* proc_list(sym_env_t* sym_env, lval_t* args, err_t* err) {
  if (NIL == args) {
    return NIL;
  }

  args->quoted = true;
  return args;
}

/*
 */
lval_t* builtin_head(sym_env_t* sym_env, lval_t* args, err_t* err) {
  // check inputs
  /*
  if( 1 != args->count ) {
    return lval_err("head accepts exactly 1 arg", err);
  }
  */

  // TODO check arg is sexp?

  lval_t* list = lval_rip(args);
  /*
  if( list->count < 1 ) {
    return lval_err("head requires a list of length at least 1", err);
  }
  */

  // by this point: x is qexp of size > 1
  lval_t* head = lval_rip(list);

  // preserve quoting for nested expressions
  head->quoted = true;  // TODO is this right?

  return head;
}

/*
 */
lval_t* builtin_tail(sym_env_t* sym_env, lval_t* x, err_t* err) {
  /*
  //check good state
  if( 1 != args->count ) {
    return lval_err("head accepts exactly 1 arg", err);
  }

  lval_t* arg1 = args->cell[0];
  if( !(arg1->count) ) {
    return lval_err("tail requires a list of length at least 1", err);
  }
  */

  x = lval_rip(x);   /* rip first arg and free the rest */
  decRef(l_pop(&x)); /* pop and discard the head */
  return x;          /* return the tail */
}

/*
 */
lval_t* builtin_join(sym_env_t* sym_env, lval_t* args, err_t* err) {
  return NULL;
}

/*
 */
lval_t* builtin_eval(sym_env_t* sym_env, lval_t* args, err_t* err) {
  return NULL;
  //##  /*
  //##  if( 1 != args->count ) {
  //##    return lval_err("eval accepts exactly 1 arg", err);
  //##  }
  //##  */

  //##  lval_t* input = lval_rip(args); // the rest of ARGS will be freed here
  //##  // TODO input = lval_pop(args); if( !sexp_empty(args) ){ lval_err(....
  //##  if (!(input->quoted)) {
  //##    return lval_err("eval expected qexp", err);
  //##  }

  //##  input->type = LVAL_SEXP;
  //##  return eval_lval(sym_env, input, err);
}

/* Note checking for overflow should be the job of LISP not here at a low
 * level
 */
lval_t* proc_sum(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);
  for (lval_t* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num += y->num;

    decRef(y);
  }

  decRef(args);
  return x;
}

/*
 */
lval_t* proc_minus(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);
  for (lval_t* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num -= y->num;

    decRef(y);
  }

  decRef(args);
  return x;
}

/*
 */
lval_t* proc_prod(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);
  for (lval_t* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num *= y->num;

    decRef(y);
  }

  decRef(args);
  return x;
}

/*
 */
lval_t* proc_quotient(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);
  for (lval_t* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num /= y->num;

    decRef(y);
  }

  decRef(args);
  return x;
}

/*
 */
lval_t* builtin_lambda(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* parameters = l_pop(&args);
  lval_t* exp = l_pop(&args);
  decRef(args);
  // TODO again, use lval_empty to check for preconditions
  // this will also involve making sure parameters and exp are not NULL
  return lval_lambda(parameters, exp, err);
}

/*
 */
lval_t* builtin_quote(sym_env_t* sym_map, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);  // WARNING not error checking
  return x;
}

/*
 */
lval_t* builtin_if(sym_env_t* sym_map, lval_t* args, err_t* err) {
  lval_t* cond = l_pop(&args);
  lval_t* a = l_pop(&args);
  lval_t* b = l_pop(&args);
  decRef(args);

  cond = eval_lval(sym_map, cond, err);
  if (scheme_true(cond)) {
    lval_clean(cond, b);
    return eval_lval(sym_map, a, err);
  } else {
    lval_clean(cond, a);
    if (b) {
      return eval_lval(sym_map, b, err);
    } else {
      return lval_undef(err);
    }
  }
}

/* TODO fix the memory semantics of this, something else has to delete the cdr
 * of the list
 */
lval_t* proc_car(sym_env_t* sym_env, lval_t* args, err_t* err) {
  if (!expect_count(args, 1)) {
    decRef(args);
    return lval_err("CAR : more than 1 arg", err);
  }

  lval_t* first = lval_rip(args);
  lval_t* x = first->car;
  incRef(x);
  decRef(first);  // TODO is this right?
  return x;
}

/*
 */
lval_t* builtin_set(sym_env_t* sym_env, lval_t* args, err_t* err) {
  /* set args */
  lval_t* variable = l_pop(&args);
  lval_t* expression = l_pop(&args);
  decRef(args);

  /* search for binding */
  char* sym = rip_sym(variable);
  binding_t* binding = sym_search(sym_env, sym);
  if (!binding) {
    char* err_msg = malloc((strlen("symbol [] is unbound") + 1 + strlen(sym)) *
                           sizeof(char));
    sprintf(err_msg, "symbol [%s] is unbound", sym);
    free(sym);
    lval_t* a = lval_err(err_msg, err);
    free(err_msg);
    return a;
  }
  free(sym);

  /* evaluate expression and set new value */
  lval_t* value = eval_lval(sym_env, expression, err);
  decRef(binding->value);
  binding->value = value;

  return lval_undef(err);
}
