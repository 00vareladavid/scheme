#include "lval_type.h"
#include "gen_util.h"
#include "lval_util.h"
#include <stdlib.h>

/* SPECIAL SCHEME VALUES */
// TODO is there a more succinct way?
static lval_t nil_x = {.type = LVAL_SPECIAL};
lval_t* NIL = &nil_x;
static lval_t l_true = {.type = LVAL_BOOL, .is_true = true};
lval_t* LISP_TRUE = &l_true;
static lval_t l_false = {.type = LVAL_BOOL, .is_true = false};
lval_t* LISP_FALSE = &l_false;

/*
*/
lval_t* new_lval(lval_type_t type, err_t* err) {
  lval_t* v = malloc(sizeof(lval_t));
  if (!v) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = type;
  v->refCount = 1;
  return v;
}

/*
*/
lval_t* lval_pair(lval_t* car, lval_t* cdr, err_t* err) {
  lval_t* v = new_lval(LVAL_PAIR, err);
  if (err->sig) {
    return lval_clean(car, cdr);
  }

  v->car = car;
  v->cdr = cdr;
  return v;
}

/*
*/
lval_t* lval_special(err_t* err) {
  lval_t* v = new_lval(LVAL_SPECIAL, err);
  if (err->sig) {
    return NULL;
  }

  return v;
}

/*
*/
lval_t* lval_num(int64_t number, err_t* err) {
  lval_t* v = new_lval(LVAL_NUM, err);
  if (err->sig) {
    return NULL;
  }

  v->num = number;
  return v;
}

/*
*/
lval_t* lval_bool(bool x, err_t* err) {
  lval_t* v = new_lval(LVAL_BOOL, err);
  if (err->sig) {
    return NULL;
  }

  v->is_true = x;
  return v;
}

/* Purpose: returns global LVAL_BOOL value
*/
lval_t* get_bool(bool x) {
  if (x) {
    return LISP_TRUE;
  }
  return LISP_FALSE;
}

/*
*/
lval_t* lval_err(char* err_msg, err_t* err) {
  lval_t* v = new_lval(LVAL_ERR, err);
  if (err->sig) {
    return NULL;
  }

  v->err = strdup(err_msg, err);
  if (err->sig) {
    return lval_clean(v, NULL);
  }
  return v;
}

/* sym_string is only read
*/
lval_t* lval_sym(char* sym_string, err_t* err) {
  lval_t* v = new_lval(LVAL_SYM, err);
  if (err->sig) {
    return NULL;
  }

  v->identifier = strdup(sym_string, err);
  if (err->sig) {
    return lval_clean(v, NULL);
  }
  return v;
}

/*
*/
lval_t* lval_builtin(builtin_fun* fun, err_t* err) {
  lval_t* v = new_lval(LVAL_PROC, err);
  if (err->sig) {
    return NULL;
  }

  v->builtin = fun;
  return v;
}

/*
*/
lval_t* lval_undef(err_t* err) {
  lval_t* v = new_lval(LVAL_UNDEF, err);
  if (err->sig) {
    return NULL;
  }
  return v;
}

/*
*/
lval_t* lval_lambda(lval_t* parameters, lval_t* exp, err_t* err) {
  lval_t* v = new_lval(LVAL_PROC, err);
  if (err->sig) {
    return lval_clean(parameters, exp);
  }

  v->builtin = NULL;
  v->parameters = parameters;
  v->exp = exp;
  v->exp->quoted = false;  // WARNING removing the quoting
  return v;
}
