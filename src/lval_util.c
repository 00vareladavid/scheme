#include "lval_util.h"
#include "gen_util.h"
#include "gc.h"

/*
*/
lval_t* ll_push(lval_t* x, lval_t* xs, err_t* err) {
  lval_t* base = lval_pair(x, xs, err);
  return base;
}

/*
*/
lval_t* ll_reverse(lval_t* xs) {
  if (NIL == xs) {
    return NIL;
  }

  /* chop */
  lval_t* old_base = xs;
  xs = xs->cdr;

  /* special case */
  old_base->cdr = NIL;

  lval_t* base;
  while (NIL != xs) {
    /* chop */
    base = xs;
    xs = xs->cdr;

    /* append */
    base->cdr = old_base;
    old_base = base;
  }

  return old_base;
}

/*
*/
char* rip_sym(lval_t* v, err_t* err) {
  char* x = strdup_e(v->identifier, err);
  decRef(v);
  return x;
}

/*
*/
lval_type_t rip_type(lval_t* v) {
  lval_type_t x = v->type;
  decRef(v);
  return x;
}

/*
*/
lval_t* lval_rip(lval_t* pair) {
  if (NIL == pair) {
    return NULL;  // this is just a signal that means nothing more to return
  }

  lval_t* x = pair->car;
  pair->car = NIL;
  decRef(pair);
  return x;
}

/* Purpose:
 * Conceptually, after all operations are done, only `base` gets a refCount
 * difference of -1
 * during a pop operation.
 * Coding it this way makes sure a pop stays O(1) instead of O(n)
*/
lval_t* l_pop(lval_t** pair) {
  if (NIL == (*pair)) {
    return NULL;  // this is just a signal that means nothing more to return
  }

  lval_t* base = *pair;
  lval_t* x = base->car;
  (*pair) = base->cdr;
  base->car = base->cdr = NIL;  // cut pointers
  decRef(base);
  return x;
}
