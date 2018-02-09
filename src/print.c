#include "print.h"
#include "lval_type.h"
#include "gen_util.h"
#include <stdio.h>

/********************************************************************************
* Internal
********************************************************************************/
/*
 */
void print_lval_func(lval_t* x) {
  if (x->builtin) {
    printf("<builtin> ");
  } else {
    printf("<lambda ");
    print_lval(x->parameters);
    print_lval(x->exp);
    printf("> ");
  }
}

/********************************************************************************
* External
********************************************************************************/
/* Purpose: dispatch print based on lval type
 * Input: lval_t is only read NOT modified
 */
void print_lval(lval_t* x) {
  /* TODO this is only here so I don't get segmentation faults right now */
  if (!x) {
    return;
  }

  // printf("[%d]", x->refCount);//debug
  lval_t* a;
  switch (x->type) {
    case LVAL_NUM:
      printf(" %ld", x->num);
      break;
    case LVAL_BOOL:
      if (x->is_true) {
        printf(" #t");
      } else {
        printf(" #f");
      }
      break;
    case LVAL_SYM:
      printf(" %s", x->identifier);
      break;
    case LVAL_ERR:
      printf("[ERROR: %s] ", x->err);
      break;
    case LVAL_PAIR:
      printf("(");
      print_lval(x->car);
      for (a = x->cdr; NIL != a; a = a->cdr) {
        print_lval(a->car);
      }
      printf(")");
      break;
    case LVAL_UNDEF:
      printf("<undef>");
      break;
    case LVAL_PROC:
      print_lval_func(x);
      break;
    case LVAL_SPECIAL:
      printf("'()");
      break;
    default:
      fucked_up("print_lval", "this type can't be printed, yo!");
      break;
  }
}
