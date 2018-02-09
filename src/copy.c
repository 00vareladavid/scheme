#include "copy.h"
#include "lval_type.h"
#include "lval_util.h"
#include "gen_util.h"

/*******************************************************************************
* Internal
*******************************************************************************/
/* copy a lambda expression
 */
lval_t* copy_lambda(lval_t* src, err_t* err) {
  lval_t* a = lval_copy(src->parameters, err);
  if (err->sig) {
    return NULL;
  }
  lval_t* b = lval_copy(src->exp, err);
  if (err->sig) {
    return lval_clean(a, NULL);
  }
  lval_t* x = lval_lambda(a, b, err);
  if (err->sig) {
    return lval_clean(a, b);
  }
  return x;
}

/*
 */
lval_t* lval_copy_func(lval_t* v, err_t* err) {
  if (v->builtin) {
    return lval_builtin(v->builtin, err);
  }

  return copy_lambda(v, err);
}

/* NOTE: this is a deep copy
 */
lval_t* lval_copy_pair(lval_t* src, err_t* err) {
  lval_t* car = lval_copy(src->car, err);
  lval_t* dest;
  if (NIL == src->cdr) {
    dest = lval_pair(car, NIL, err);
  } else {
    lval_t* cdr = lval_copy(src->cdr, err);
    dest = lval_pair(car, cdr, err);
  }
  return dest;
}

/*******************************************************************************
* External
*******************************************************************************/
/* purpose: dispatch copy function based on lval type
 */
lval_t* lval_copy(lval_t* v, err_t* err) {
  if (!v) {
    return NULL;
  }

  switch (v->type) {
    case LVAL_UNDEF:
      return lval_undef(err);
    case LVAL_NUM:
      return lval_num(v->num, err);
    case LVAL_BOOL:
      return v;
    case LVAL_PROC:
      return lval_copy_func(v, err);
    case LVAL_SYM:
      return lval_sym(v->identifier, err);
    case LVAL_PAIR:
      return lval_copy_pair(v, err);
    default:
      fucked_up("lval_copy", "this type doesn't exist, yo!");
  }
  return NULL; /* should not be reached, just to shut up compiler */
}
