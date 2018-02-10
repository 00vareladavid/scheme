#pragma once
#include "decl.h"
#include <stdbool.h>

/********************************************************************************
* TYPE DEF
********************************************************************************/
/* 
 */
struct binding_t;
typedef struct binding_t binding_t;

/*
 */
struct binding_t {
  char* key;
  lval_t* value;
  binding_t* sibling;
};

/*
*/
struct sym_env_t {
  binding_t *first_child, *last_child;
  sym_env_t* parent;
};

/*******************************************************************************
* DECLARATIONS
*******************************************************************************/
/* Does not modify anything, merely returns a pointer
 */
binding_t* sym_search(sym_env_t* env, char* key);

/* note: this is the correct interface function to use (not `kv_push`)
 * consumes: symbol, value
 */
bool symbol_add(sym_env_t* sym_map, char* symbol, lval_t* value, err_t* err);

/*
 */
bool global_symbol_add(char* symbol, lval_t* value, err_t* err);

/* note: this will delete kv-chain but not parent
 */
void sym_map_del(sym_env_t* sym_map);

/*
 */
sym_env_t* sym_map_make(err_t* err);

/*
 */
sym_env_t* simple_populate(lval_t* parameters, lval_t* args, err_t* err);

/*
 */
sym_env_t* get_global_map(void);

/*
 */
void set_global_map(sym_env_t* x);

