#include "repl.h"
#include "gen_util.h"
#include "print.h"
#include "linenoise/linenoise.h"
#include <stdio.h>
//TODO get rid of `string.h` when you moved to ctrl+c instead of QUIT
#include <string.h> 

/*******************************************************************************
* INTERNAL
*******************************************************************************/

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
