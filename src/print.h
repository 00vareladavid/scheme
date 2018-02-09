#pragma once

/*************************************** 
* Forward decl
***************************************/
struct lval_t;
typedef struct lval_t lval_t;

/*************************************** 
* DEFS
***************************************/
/* Purpose: dispatch print based on lval type
 * Input: lval_t is only read NOT modified
 */
void print_lval(lval_t* x);
