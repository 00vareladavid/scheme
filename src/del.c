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

/* Free the memory used by up to three lvals
*/
lval_t* lval_clean(lval_t* a, lval_t* b) {
  decRef(a);
  decRef(b);
  return NULL;
}
