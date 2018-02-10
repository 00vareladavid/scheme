/* Scheme implementation
 */
#include "sym_env.h"
#include "setup.h"
#include "repl.h"
#include <stdlib.h> //this is only needed for debugging: add macro flags

/*
 */
int main(void) {
  /* Initialize */
  err_t* err = set_base_err();
  init_global_map(err);

  sym_env_t* sym_env = get_global_map();  // dirty hack to satisfy type system for now

  /* Load STDLIB */
  load_stdlib(sym_env, err);

  /* REPL */
  repl(sym_env, err);

  /* CLEAN */
  /* TODO this is only needed when checking for memory errors
   * -> add macro flags
   */
  sym_map_del(get_global_map());
  free(err);
  return 0;
}
