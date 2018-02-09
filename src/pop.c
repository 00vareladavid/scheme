/* WARNING: this function assumes:
** * parameters->count == args->count
** * parameters is a sexp with only LVAL_SYM as children
** * args is a sexp
** ---
** consumes parameters
** consumes args
*/
sym_env_t* populate(sym_env_t* parent,
                    lval_t* parameters,
                    lval_t* args,
                    err_t* err) {
  /* create blank slate; */
  sym_env_t* sym_map = sym_map_make(err);
  if (err->sig) {
    lval_clean(args, parameters);
    return NULL;
  }
  sym_map->parent = parent;

  /* populate */
  char* sym;
  for (lval_t* a = l_pop(&parameters); a; a = l_pop(&parameters)) {
    sym = rip_sym(a);

    /* handle variable arguments */
    if (!strcmp(sym, "&")) {
      free(sym);  // discard
      return populate_rest(sym_map, parameters, args, err);
    }

    kv_push(sym_map, sym, l_pop(&args), err);
    if (err->sig) {
      puts("pop sym env");
      exit(1);
    }
  }

  /* clean, push, return */
  lval_clean(args, parameters);
  return sym_map;
}
