#include "repl.h"
#include "gen_util.h"
#include "print.h"
#include "parse.h"
#include "linenoise.h"
#include "gc.h"
#include "eval.h"
#include <stdio.h>
//TODO get rid of `string.h` when you moved to ctrl+c instead of QUIT
#include <string.h> 

/*******************************************************************************
* INTERNAL
*******************************************************************************/
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

/*
 */
char* prompt(void) {
  puts("\n+--------------------------------------+");
  char* input = linenoise("> ");
  linenoiseHistoryAdd(input);
  return input;
}

/*******************************************************************************
* EXTERNAL
*******************************************************************************/

/*
 */
void repl(sym_env_t* sym_env, err_t* err) {
  puts("Scheme v0.1.0");
  puts("Enter 'EXIT' to exit");

  lval_t* x;
  char* input = prompt();
  while (strcmp(input, "QUIT")) {
    x = lisp_exec(sym_env, input, err);
    /* if error: reset signal and skip printing */
    if (err->sig) {  
      /* TODO I am throwing all kinds of errors together, seperate them */
      err->sig = OK;
    } else {
      print_lval(x);
      decRef(x);
    }
    input = prompt();
  }
  linenoiseFree(input);
}
