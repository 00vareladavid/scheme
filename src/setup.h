#pragma once
#include "decl.h"

/*
 */
err_t* set_base_err(void);

/*
 */
void init_global_map(err_t* err);

/*
 */
void load_stdlib(sym_env_t* map, err_t* err);
