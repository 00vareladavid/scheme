#include "features.h"
#include "file.h"
#include "parse.h"
#include "eval.h"
#include "gc.h"
#include <stdlib.h>

/*
*/
void exec_file(sym_env_t* sym_env, char* filename, err_t* err) {
  char* code = (char*)read_whole_file(filename);
  lval_t* x = parse_all(code, err);
  x = eval_lval(sym_env, x, err);
  decRef(x); /* discard, only executing for side effects */
  free(code);
}
