/*
*/
sym_env_t* populate_rest(sym_env_t* sym_map,
                         lval_t* parameters,
                         lval_t* args,
                         err_t* err) {
  char* sym = rip_sym(lval_rip(parameters));
  args->quoted = true;  // TODO should this be higher up the chain?

  kv_push(sym_map, sym, args, err);
  if (err->sig) {
    puts("pop sym env");
    exit(1);
  }
  return sym_map;
}
