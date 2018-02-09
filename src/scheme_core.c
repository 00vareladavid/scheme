#include "scheme_core.h"
#include "parse.h"
#include "mem_err.h"
#include "lval_util.h"
#include <stdio.h>

/*
 */
lval_t* lisp_exec(sym_env_t* sym_env, char* input, err_t* err) {
  /* read */
  lval_t* x = parse_one(input, err);
  // TODO check for bad parse
  if (err->sig) {
    printf("[ERR] Unable to READ lval_t due to insufficient memory\n");
    return lval_clean(x, NULL);
  }

  /* eval */
  x = eval_lval(sym_env, x, err);
  if (err->sig) {
    printf("[ERR] Unable to EVAL lval due to insufficient memory\n");
    return lval_clean(x, NULL);
  }

  return x;
}
