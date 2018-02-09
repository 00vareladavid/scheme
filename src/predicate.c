#include "predicate.h"
#include "gen_util.h"
#include "proc_util.h"
#include "lval_type.h"
#include "lval_util.h"
#include "gc.h"
#include <string.h>

/*******************************************************************************
* Internal
*******************************************************************************/
/*
 */
lval_t* scheme_type_predicate(lval_type_t target, lval_t* x) {
  bool a = ( target == x->type );
  decRef(x);
  return get_bool(a);
}

/*******************************************************************************
* External
*******************************************************************************/
/*
 */
lval_t* proc_pred_bool(sym_env_t* sym_map, lval_t* args, err_t* err) {
  silence(sym_map);//TODO compiler
  lval_t* x;
  if( extract_arg(args,1,&x) ){
    return lval_err("proc_pred_bool: unexpected number of args", err);
  }

  return scheme_type_predicate(LVAL_BOOL, x);
}

/*
 */
lval_t* proc_pred_num(sym_env_t* sym_map, lval_t* args, err_t* err) {
  silence(sym_map);//TODO compiler
  lval_t* x;
  if( extract_arg(args,1,&x) ){
    return lval_err("proc_pred_num: unexpected number of args", err);
  }

  return scheme_type_predicate(LVAL_NUM, x);
}

/*
 */
lval_t* proc_pred_sym(sym_env_t* sym_map, lval_t* args, err_t* err) {
  silence(sym_map);//TODO compiler
  lval_t* x;
  if( extract_arg(args,1,&x) ){
    return lval_err("proc_pred_sym: unexpected number of args", err);
  }

  return scheme_type_predicate(LVAL_SYM, x);
}

/*
 */
lval_t* proc_pred_fun(sym_env_t* sym_map, lval_t* args, err_t* err) {
  silence(sym_map);//TODO compiler
  lval_t* x;
  if( extract_arg(args,1,&x) ){
    return lval_err("proc_pred_fun: unexpected number of args", err);
  }

  return scheme_type_predicate(LVAL_PROC, x);
}

/*
 */
lval_t* proc_pred_pair(sym_env_t* sym_map, lval_t* args, err_t* err) {
  silence(sym_map);//TODO compiler
  lval_t* x;
  if( extract_arg(args,1,&x) ){
    return lval_err("proc_pred_pair: unexpected number of args", err);
  }

  return scheme_type_predicate(LVAL_PAIR, x);
}

/*
 */
lval_t* proc_pred_eqv(sym_env_t* sym_map, lval_t* args, err_t* err) {
  silence(sym_map);//TODO compiler
  lval_t *a, *b;
  if( extract_arg(args,2,&a,&b) ){
    return lval_err("proc_pred_eqv: unexpected number of args", err);
  }

  /* deal with empty list */
  if ((NIL == a) && (NIL == b)) {
    return LISP_TRUE;
  }

  /* if not same type, can't be  eqv */
  if (a->type != b->type) {
    lval_clean(a, b);
    return LISP_FALSE;
  }

  bool x = false;
  switch (a->type) {
    case LVAL_BOOL:
      x = (a->is_true == b->is_true);
      break;
    case LVAL_SYM:
      x = !strcmp(a->identifier, b->identifier);
      break;
    case LVAL_NUM:
      x = (a->num == b->num);
      break;
    case LVAL_PAIR:
      x = (a == b);
      break;
    case LVAL_PROC:
      x = (a->builtin == b->builtin);
      break;
    default:
      fucked_up("proc_pred_eqv", "type is not recognized yo");
  }

  lval_clean(a, b);
  return get_bool(x);
}
