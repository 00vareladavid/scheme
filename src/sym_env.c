#include "sym_env.h"
#include "gen_util.h"
#include "lval_util.h"
#include "gc.h"
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
* GLOBAL
*******************************************************************************/
static sym_env_t* SYM_MAP;//TODO is `static` correct here?

/*******************************************************************************
* Internal
*******************************************************************************/
/* consumes key
 * consumes value
 * modifies sym_map
 * modifies error
 */
void kv_push(sym_env_t* sym_map, char* key, lval_t* value, err_t* err) {
  /* allocate space */
  binding_t* kv = malloc(sizeof(binding_t));
  if (!kv) {
    free(key);
    decRef(value);
    err->sig = OUT_OF_MEM;
    return;
  }

  /* set values */
  kv->key = key;
  kv->value = value;
  // incRef(kv->value);
  kv->sibling = NULL;

  /* append */
  if (!(sym_map->first_child)) {
    sym_map->first_child = kv;
    sym_map->last_child = kv;
  } else {
    sym_map->last_child->sibling = kv;
    sym_map->last_child = kv;
  }
}

/*
*/
sym_env_t* populate_rest(sym_env_t* sym_map,
                         lval_t* parameters,
                         lval_t* args,
                         err_t* err) {
  char* sym = rip_sym(lval_rip(parameters), err);
  args->quoted = true;  // TODO should this be higher up the chain?

  kv_push(sym_map, sym, args, err);
  if (err->sig) {
    fucked_up("populate_rest", "pop sym env");
  }
  return sym_map;
}

/* WARNING: this function assumes:
 * * parameters->count == args->count
 * * parameters is a sexp with only LVAL_SYM as children
 * * args is a sexp
 * ---
 * consumes parameters
 * consumes args
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
    sym = rip_sym(a, err);

    /* handle variable arguments */
    if (!strcmp(sym, "&")) {
      free(sym);  // discard
      return populate_rest(sym_map, parameters, args, err);
    }

    kv_push(sym_map, sym, l_pop(&args), err);
    if (err->sig) {
      fucked_up("populate","idk why");
    }
  }

  /* clean, push, return */
  lval_clean(args, parameters);
  return sym_map;
}

/*******************************************************************************
* External
*******************************************************************************/
/* Does not modify anything, merely returns a pointer
 */
binding_t* sym_search(sym_env_t* env, char* key) {
  for (binding_t* x = env->first_child; x; x = x->sibling) {
    if (!strcmp(x->key, key)) {  // hit!
      return x;
    }
  }

  /* not in this level, search higher if possible */
  if (env->parent) {
    return sym_search(env->parent, key);
  }
  return NULL;
}

/* note: this is the correct interface function to use (not `kv_push`)
 * consumes: symbol, value
 */
bool symbol_add(sym_env_t* sym_map, char* symbol, lval_t* value, err_t* err) {
  // make sure there is something in the sym_env_t before you being searching
  if (sym_map && sym_search(sym_map, symbol)) {
    free(symbol);
    decRef(value);
    return true;
  }

  kv_push(sym_map, symbol, value, err);
  return false;
}

/*
 */
bool global_symbol_add(char* symbol, lval_t* value, err_t* err) {
  return symbol_add(SYM_MAP, symbol, value, err);
}

/* note: this will delete kv-chain but not parent
 */
void sym_map_del(sym_env_t* sym_map) {
  binding_t *next_binding, *binding = sym_map->first_child;
  while (binding) {
    next_binding = binding->sibling;
    free(binding->key);
    decRef(binding->value);
    free(binding);
    binding = next_binding;
  }

  free(sym_map);
}

/*
 */
sym_env_t* sym_map_make(err_t* err) {
  sym_env_t* sym_map = malloc(sizeof(sym_env_t));
  if (!sym_map) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  sym_map->first_child = sym_map->last_child = NULL;
  sym_map->parent = NULL;
  return sym_map;
}

/*
 */
sym_env_t* simple_populate( lval_t* parameters, lval_t* args, err_t* err) {
  return populate(SYM_MAP, parameters, args, err);
}

/*
 */
sym_env_t* get_global_map(void) {
  return SYM_MAP;
}

/*
 */
void set_global_map(sym_env_t* x) {
  SYM_MAP = x;
}
