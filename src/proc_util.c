#include "proc_util.h"
#include "lval_util.h"
#include <stdarg.h>

/* Purpose: extract an exact `num` of `args` into the specified locations
 * Errors:
 *  - not enough args -> false
 *  - too many args -> false
 *  - num != va_list size -> fucked_up
 * TODO error handling
 */
bool extract_arg(lval_t* args, uint16_t num, ...) {
  va_list params;
  va_start(params, num);

  lval_t** next_loc;
  for(uint16_t count = 0; count < num; ++count) {
    /* pop next argument into the correct location */
    next_loc = va_arg(params, lval_t**);
    *next_loc = l_pop(&args);
  }
  return true;
}

/* Purpose: silence compiler warnings for now
 */
bool silence(void* sym_env) {
  return 0 == sym_env;
}
