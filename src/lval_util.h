#pragma once
#include "lval_type.h"

/***************************************
* Forward decl
***************************************/
struct err_t;
typedef struct err_t err_t;

/***************************************
* Defs
***************************************/

/*
*/
lval_t* ll_push(lval_t* x, lval_t* xs, err_t* err); 

/*
*/
lval_t* ll_reverse(lval_t* xs); 

/*
*/
char* rip_sym(lval_t* v, err_t* err); 

/*
*/
lval_type_t rip_type(lval_t* v); 

/* Free the memory used by up to two lvals
*/
lval_t* lval_clean(lval_t* a, lval_t* b); 

/*
*/
lval_t* lval_rip(lval_t* pair); 

/* Purpose:
 * Conceptually, after all operations are done, only `base` gets a refCount
 * difference of -1
 * during a pop operation.
 * Coding it this way makes sure a pop stays O(1) instead of O(n)
*/
lval_t* l_pop(lval_t** pair); 
