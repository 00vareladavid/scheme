#include "gc.h"
#include "lval_type.h"
/* fucked_up */
#include "gen_util.h"
/* free */
#include <stdlib.h>

/*******************************************************************************
* Internal
*******************************************************************************/
/* WARNING: this does not delete the struct itself, only the approriate elements
 */
void lval_del_func(lval_t* v) {
  /* if a lambda expression */
  if (!(v->builtin)) {
    decRef(v->exp);
    decRef(v->parameters);
  }
  /* nothing to delete for builtin functions */
}

/* deletes the immediate lval structure
 * WARNING: does no semantic checking
 * WARNING: does not act recursively
 */
void lval_del(lval_t* v) {
  /* no-op on NULL */
  if (!v) {
    return;
  }

  switch (v->type) {
    case LVAL_NUM:
    case LVAL_BOOL:
    case LVAL_SPECIAL:
    case LVAL_UNDEF:
    case LVAL_PAIR:
      break;
    case LVAL_ERR:
      free(v->err);
      break;
    case LVAL_SYM:
      free(v->identifier);
      break;
    case LVAL_PROC:
      lval_del_func(v);
      break;
    default:
      fucked_up("lval_del", "I dont recognize this type yo");
      break;
  }
  free(v);
}

/*
 */
void gc_attempt(lval_t* x) {
  switch (x->type) {
    case LVAL_PAIR:
    case LVAL_NUM:
    case LVAL_SYM:
    case LVAL_UNDEF:
    case LVAL_ERR:
    case LVAL_PROC:
      if (!(x->refCount)) {
        lval_del(x);
      }
    /* special object for which it is only necessary to allocate a single global
     * object */
    case LVAL_SPECIAL:
    case LVAL_BOOL:
      break;
  }
}

/*******************************************************************************
* External
*******************************************************************************/

/* TODO make sure NIL is never freed
 * TODO make sure that refCount does not overflow!
 */
lval_t* incRef(lval_t* x) {
  switch (x->type) {
    case LVAL_PAIR:
      incRef(x->car);
      incRef(x->cdr);
    case LVAL_NUM:
    case LVAL_SYM:
    case LVAL_UNDEF:
    case LVAL_ERR:
    case LVAL_PROC:
      x->refCount++;
    case LVAL_SPECIAL:
    case LVAL_BOOL:
      break;
  }

  return x;
}

/* This will first reduce the reference count for each object
 * Then it will free any objects which are part of that structure
 */
void decRef(lval_t* x) {
  if (!x) {
    return;
  }

  switch (x->type) {
    case LVAL_PAIR:
      decRef(x->car);
      decRef(x->cdr);
    case LVAL_NUM:
    case LVAL_SYM:
    case LVAL_UNDEF:
    case LVAL_ERR:
    case LVAL_PROC:
      x->refCount--;
      gc_attempt(x);
    case LVAL_SPECIAL:
    case LVAL_BOOL:
      break;
  }
}

/*
 */
lval_t* lval_clean(lval_t* a, lval_t* b) {
  decRef(a);
  decRef(b);
  return NULL;
}
