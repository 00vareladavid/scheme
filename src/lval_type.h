#pragma once
#include "decl.h"
#include "sym_env.h"
#include <stdint.h>
#include <stdbool.h>

/*
*/
enum lval_type_t {
  LVAL_NUM,
  LVAL_SYM,
  LVAL_PAIR,
  LVAL_BOOL,
  LVAL_PROC,
  LVAL_SPECIAL,

  LVAL_ERR,
  LVAL_UNDEF,
};
typedef enum lval_type_t lval_type_t;

/*
*/
typedef lval_t*(builtin_fun)(sym_env_t*, lval_t*, err_t*);

struct lval_t {
  lval_type_t type;
  bool quoted;
  uint16_t refCount;

  union {
    int64_t num;      /* num */
    bool is_true;     /* boolean */
    char* identifier; /* symbol */
    struct {
      lval_t* car;
      lval_t* cdr;
    };
    struct {
      builtin_fun* builtin;
      lval_t* parameters;
      lval_t* exp;
    };

    char* err; /* err */
  };
};

/* SPECIAL SCHEME VALUES */
extern lval_t* NIL;
extern lval_t* LISP_TRUE;
extern lval_t* LISP_FALSE;

/*
*/
lval_t* new_lval(lval_type_t type, err_t* err);
lval_t* lval_special(err_t* err);
lval_t* lval_num(int64_t x, err_t* err);
lval_t* get_bool(bool x);
lval_t* lval_bool(bool x, err_t* err);
lval_t* lval_err(char* err_msg, err_t* err);
lval_t* lval_sym(char* sym_string, err_t* err);
lval_t* lval_pair(lval_t* car, lval_t* cdr, err_t* err);
lval_t* lval_undef(err_t* err);
lval_t* lval_lambda(lval_t* parameters, lval_t* exp, err_t* err);
lval_t* lval_builtin(builtin_fun* fun, err_t* err);
void lval_del(lval_t* x);
