#pragma once

/* forward dec */
struct lval_t;
typedef struct lval_t lval_t;

lval_t* incRef(lval_t* x);
void decRef(lval_t* x);
lval_t* lval_clean(lval_t* a, lval_t* b);
