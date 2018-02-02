/*
--------------------------------------------------------------------------------
* all eval functions will mooch of of Y
* all symbol table functions should take to deal with a copy
*TODO run valgrind to find memroy err
 * run on symbols until error occurs
*TODO refactor before addding other features
--------------------------------------------------------------------------------
# Questions
* what doesn't make sense to copy over?
--------------------------------------------------------------------------------
# Specific Notes
* dont use lvals which where read by READ as the whole tree will be freed by the
  end of the REPL
--------------------------------------------------------------------------------
# General Notes
* always use sizeof with malloc
* copy by default
* decide who is responsible for what
 * do you expect this arg to be unchanged?
 * is the return value meant to be only accessed or modified?
* use enumerated unsigned vals as the return value for erro handling
* to make memory easy, just copy everything
 * later you can see what makes sense to copy and what makes sense to destroy
## Memory management
* functions are responsible for their inputs
* reuse the parts you need and free the parts you dont
* hide memory management through the use of utility functions
* make sure you store any relevant values before you delete
** this will save you from invalid read
## Function structure
* assertions
 * use control flow to prevent superfluous assertions
* accesing/manipulation
* check for error conditions
* allocate/free memory
 * clearly specify who is responsible for what memory
* don't mix static and dynamic pointers
 * ie dynamically allocated strings and strings on the heap
## Error Handling
* after every xalloc, handle the split between 'enough mem' and 'out of mem'
* only build one semantic peiece at a time, if a memory error occurs, you will
know what to clean up
### If you caught a memory error
* free any state you consume
* return NULL
### Error Types
* lval -> destroy lval
* sym_tab?
## Refactoring
* first set the type system in a MVP
 * then flesh out the type system with code
* remember to balance top-down with bottom-up
--------------------------------------------------------------------------------
# After
* review C build process
* data structures and algorithms
--------------------------------------------------------------------------------
# Further reasearch
*/
#include "linenoise/linenoise.h"
#include "mpc/mpc.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/********************************************************************************
* UTILS
********************************************************************************/
// int mallocfail = -5000;
//#define oldmalloc(x) (malloc(x))
//#define malloc(x) ((1 == ++mallocfail) ? NULL : oldmalloc(x))

/*
*/
void fucked_up(char* function_name, char* err_msg) {
  fprintf(stderr, "[ERROR] %s: %s\n", function_name, err_msg);
  exit(1);
}

/* TODO verify the correctness of this function
** TODO replace calloc with this malloc wraper
** TODO learn more about valgrind
*/
void* x_malloc(size_t num, size_t size) {
  if (!size || !num) {
    return malloc(0);
  }

  size_t total = num * size;
  if (total < num) {
    fucked_up("malloc", "integer oveflow");
  }

  return malloc(total);
}

size_t inc_size(size_t x) {
  x += 1u;

  if (!x) {
    fucked_up("inc_size", "unsigned carry(overflow)");
  }

  return x;
}

/********************************************************************************
* TYPES
********************************************************************************/
/* TODO learn about forward declarations
*/

struct binding_t;
typedef struct binding_t binding_t;
struct symbol_map;
typedef struct symbol_map symbol_map;
struct err_t;
typedef struct err_t err_t;
struct lval;
typedef struct lval lval;

typedef lval*(builtin_fun)(symbol_map*, lval*, err_t*);

/*
*/
struct binding_t {
  char* key;
  lval* value;
  binding_t* sibling;
};

/*
*/
struct symbol_map {
  binding_t *first_child, *last_child;
  symbol_map* parent;
};

/*
*/
enum err_sig_t { OK, OUT_OF_MEM, BAD_PARSE };
typedef enum err_sig_t err_sig_t;

struct err_t {
  err_sig_t sig;
};

/*
*/
enum lval_type_t {
  LVAL_NUM,
  LVAL_PAIR,
  LVAL_BOOL,
  LVAL_ERR,
  LVAL_SYM,
  LVAL_PROC,
  LVAL_UNDEF,
};
typedef enum lval_type_t lval_type_t;

/*
*/
struct lval {
  lval_type_t type;
  bool quoted;

  int64_t num; /* num */
  char* err;   /* err */
  char* sym;   /* symbol */

  /* boolean */
  bool is_true;

  /* functions */

  /* - builtin */
  builtin_fun* builtin;
  /* - lambda */
  lval* parameters;
  lval* exp;

  /* pair */
  lval* car;
  lval* cdr;
};
typedef struct lval lval;

/********************************************************************************
* PROTOTYPES
********************************************************************************/
// debugging
char* lval_type_string(lval_type_t);

// main
char* prompt(void);
void repl(mpc_parser_t* Lispy, symbol_map* sym_env, err_t* err);

// constructors
lval* lval_num(int64_t x, err_t* err);
lval* lval_err(char* err_msg, err_t* err);
lval* lval_sym(char* sym_string, err_t* err);
lval* lval_undef(err_t* err);
lval* lval_lambda(lval* parameters, lval* exp, err_t* err);

// lval utils
char* rip_sym(lval* v);

// lval del
void lval_del(lval* v);
lval* lval_clean(lval* a, lval* b, lval* c);

// lval copy
lval* lval_copy(lval* v, err_t* err);

// lval read
lval* lisp_read(mpc_ast_t* t, err_t* err);
lval* read_lval(mpc_ast_t* t, err_t* err);
lval* read_children(lval* parent, mpc_ast_t* t, err_t* err);
lval* read_num(mpc_ast_t* t, err_t* err);

// lval print
void print(lval* x);
void print_lval(lval* x);

// lval eval
lval* eval_lval(symbol_map*, lval* v, err_t* err);
lval* eval_symbol(symbol_map* sym_env, lval* x, err_t* err);
lval* dispatch_lambda(symbol_map*, lval* func, lval* args, err_t* err);

/* BUILTINS */

/* most complex builtins, require all three args */
lval* builtin_define(symbol_map* sym_env, lval* args, err_t* err);
lval* builtin_set(symbol_map* sym_env, lval* args, err_t* err);
lval* builtin_eval(symbol_map* sym_env, lval* args, err_t* err);

/* these builtins operate directly on their args, no sym_env needed */
lval* builtin_lambda(symbol_map* sym_env, lval* args, err_t* err);
lval* builtin_head(symbol_map* sym_env, lval* args, err_t* err);
lval* builtin_tail(symbol_map* sym_env, lval* args, err_t* err);
lval* builtin_join(symbol_map* sym_env, lval* args, err_t* err);

/* safe builtins, no chance of memory errors */
lval* builtin_quote(symbol_map* sym_env, lval* args, err_t* err);
lval* builtin_if(symbol_map* sym_map, lval* args, err_t* err);

lval* proc_pred_bool(symbol_map* sym_map, lval* args, err_t* err);
lval* proc_pred_num(symbol_map* sym_map, lval* args, err_t* err);
lval* proc_pred_sym(symbol_map* sym_map, lval* args, err_t* err);
lval* proc_pred_fun(symbol_map* sym_map, lval* args, err_t* err);
lval* proc_pred_pair(symbol_map* sym_map, lval* args, err_t* err);

lval* proc_pred_eqv(symbol_map* sym_map, lval* args, err_t* err);

lval* proc_car(symbol_map* sym_map, lval* args, err_t* err);
lval* builtin_list(symbol_map* sym_env, lval* args, err_t* err);
lval* proc_sum(symbol_map* sym_env, lval* args, err_t* err);
lval* proc_minus(symbol_map* sym_env, lval* args, err_t* err);
lval* proc_prod(symbol_map* sym_env, lval* args, err_t* err);
lval* proc_quotient(symbol_map* sym_env, lval* args, err_t* err);

/* symbols */
symbol_map* init_global_map(err_t* err);
void push_builtin(symbol_map* sym_map, char* fun_name, err_t* err);
bool symbol_add(symbol_map* sym_map, char* symbol, lval* value, err_t* err);
void kv_push(symbol_map* sym_map, char* key, lval* value, err_t* err);
symbol_map* sym_map_make(err_t* err);
void sym_map_del(symbol_map* sym_map);
builtin_fun* proc_tab(char* x);
binding_t* sym_search(symbol_map* env, char* key);
symbol_map* populate(symbol_map* parent,
                     lval* parameters,
                     lval* args,
                     err_t* err);

/********************************************************************************
* SHOULD BE IN string.h
********************************************************************************/
char* strdup(char* input, err_t* err) {
  char* x = calloc(inc_size(strlen(input)), sizeof(char));
  if (!x) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  strcpy(x, input);
  return x;
}

/*
*/
symbol_map* SYM_MAP;
lval* EMPTY_LIST;

/********************************************************************************
* MAIN
********************************************************************************/
int main(int argc, char* argv[]) {
  /*** INIT ***/
  /* TODO make sure this is not null and it is never changed */
  EMPTY_LIST = malloc(sizeof(lval));

  /* error struct */
  err_t* err = malloc(sizeof(err_t));
  if (!err) {
    printf("1 insufficient mem for baseline framework\n");
    exit(1);
  }
  err->sig = OK;

  /* symbol env */
  SYM_MAP = init_global_map(err);
  if (err->sig) {
    free(err);
    printf("2 insufficient mem for baseline framework\n");
    exit(1);
  }
  symbol_map* sym_env = SYM_MAP;  // dirty hack to satisfy type system for now

  /* parser */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* List = mpc_new("list");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
            " \
       number : /-?[0-9]+/ ;\
       symbol : '+' | '-' | '*' | '/' | /[a-zA-Z0-0_&#?!]+/ ;\
       list : '(' <expr>* ')' ;\
       expr : <number> | <symbol> | <list> ;\
       lispy : /^/ <expr>+ /$/ ;\
     ",
            Number, Symbol, List, Expr, Lispy);

  /*** LOAD STDLIB ***/
  // stdlib(sym_env, err);

  /*** REPL ***/
  repl(Lispy, sym_env, err);

  /*** CLEAN ***/
  free(err);
  free(EMPTY_LIST);
  mpc_cleanup(5, Number, Symbol, List, Expr, Lispy);
  sym_map_del(SYM_MAP);
  return 0;
}

/*******************************************************************************
* Standard Library
*******************************************************************************/
lval* lisp_exec(mpc_parser_t* Lispy,
                symbol_map* sym_env,
                char* input,
                err_t* err) {
  lval* x = NULL;
  mpc_result_t r; /* holds the ast */
  if (mpc_parse("<stdin>", input, Lispy, &r)) {
    /* read */
    x = lisp_read(r.output, err);
    if (err->sig) {
      printf("[ERR] Unable to READ lval due to insufficient memory\n");
      return lval_clean(x, NULL, NULL);
    }

    /* eval */
    x = eval_lval(sym_env, x, err);
    if (err->sig) {
      printf("[ERR] Unable to EVAL lval due to insufficient memory\n");
      return lval_clean(x, NULL, NULL);
    }
  } else {
    /* bad parse */
    puts("Bad Parse:");
    err->sig = BAD_PARSE;
    mpc_err_print(r.error);
    mpc_err_delete(r.error);
  }

  free(input);
  return x;
}

void stdlib(mpc_parser_t* Lispy, symbol_map* sym_env, err_t* err) {
  // char** code = ["(define {fun} (lambda {args body} {define (head args)
  // lisp_exec(Lispy, sym_env,
}

/*******************************************************************************
* Interactive Prompt
*******************************************************************************/

/*
*/
char* prompt(void) {
  puts("\n+--------------------------------------+");
  char* input = linenoise("> ");
  linenoiseHistoryAdd(input);
  return input;
}

/*
*/
void repl(mpc_parser_t* Lispy, symbol_map* sym_env, err_t* err) {
  puts("Lispy v0.1.0");
  puts("Enter 'EXIT' to exit");

  // mpc_result_t r; /* holds the ast */
  lval* x;
  char* input = prompt();
  while (strcmp(input, "QUIT")) {
    x = lisp_exec(Lispy, sym_env, input, err);
    /* if error: reset signal and skip printing */
    if (err->sig) {  // TODO I am throwing all kinds of errors together,
                     // seperate them
      err->sig = OK;
    } else {
      print(x);
    }
    input = prompt();
  }
  free(input);
}

/********************************************************************************
* CONSTRUCTORS
********************************************************************************/
lval* lval_pair(lval* car, lval* cdr, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if (!v) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_PAIR;
  v->car = car;
  v->cdr = cdr;
  return v;
}

lval* lval_num(int64_t number, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if (!v) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_NUM;
  v->num = number;
  return v;
}

/*
*/
lval* lval_bool(bool x, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if (!v) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_BOOL;
  v->is_true = x;
  return v;
}

/*
*/
lval* lval_err(char* err_msg, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if (!v) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_ERR;
  v->err = strdup(err_msg, err);
  if (err->sig) {
    return lval_clean(v, NULL, NULL);
  }

  return v;
}

/* sym_string is only read
*/
lval* lval_sym(char* sym_string, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if (!v) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_SYM;
  v->sym = strdup(sym_string, err);
  if (err->sig) {
    return lval_clean(v, NULL, NULL);
  }

  return v;
}

//======================================
lval* lval_builtin(builtin_fun* fun, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if (!v) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_PROC;
  v->builtin = fun;
  return v;
}

//======================================
lval* lval_undef(err_t* err) {
  lval* x = malloc(sizeof(lval));
  if (!x) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  x->type = LVAL_UNDEF;
  return x;
}

//======================================
lval* lval_lambda(lval* parameters, lval* exp, err_t* err) {
  lval* x = malloc(sizeof(lval));
  if (!x) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  x->type = LVAL_PROC;
  x->builtin = NULL;
  x->parameters = parameters;
  x->exp = exp;
  x->exp->quoted = false;  // WARNING removing the quoting
  return x;
}

/********************************************************************************
* LVAL UTILS
********************************************************************************/
/*
*/
char* rip_sym(lval* v) {
  char* x = v->sym;
  v->sym = NULL;
  lval_del(v);
  return x;
}

lval_type_t rip_type(lval* v) {
  lval_type_t x = v->type;
  lval_del(v);
  return x;
}

/*******************************************************************************
* DELETE
*******************************************************************************/
/* WARNING: this does not delete the struct itself, only the approriate elements
*/
void lval_del_func(lval* v) {
  /* if a lambda expression */
  if (!(v->builtin)) {
    lval_del(v->exp);
    lval_del(v->parameters);
  }

  /* nothing to delete for builtin functions */
}

/* recursively free the memory used by an lval
*/
void lval_del(lval* v) {
  /* no-op on NULL */
  if (!v) {
    return;
  }
  if (EMPTY_LIST == v) {
    return;
  }

  switch (v->type) {
    case LVAL_NUM:
    case LVAL_BOOL:
    case LVAL_UNDEF:
      break; /* do nothing */
    case LVAL_ERR:
      free(v->err);
      break;
    case LVAL_SYM:
      free(v->sym);
      break;
    case LVAL_PAIR:
      lval_del(v->car);
      lval_del(v->cdr);
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
lval* lval_clean(lval* a, lval* b, lval* c) {
  lval_del(a);
  lval_del(b);
  lval_del(c);
  return NULL;
}

/*******************************************************************************
* COPY
*******************************************************************************/

/* copy a lambda expression
*/
lval* copy_lambda(lval* src, err_t* err) {
  lval* a = lval_copy(src->parameters, err);
  if (err->sig) {
    return NULL;
  }
  lval* b = lval_copy(src->exp, err);
  if (err->sig) {
    return lval_clean(a, NULL, NULL);
  }
  lval* x = lval_lambda(a, b, err);
  if (err->sig) {
    return lval_clean(a, b, NULL);
  }
  return x;
}

/*
*/
lval* lval_copy_func(lval* v, err_t* err) {
  if (v->builtin) {
    return lval_builtin(v->builtin, err);
  }

  return copy_lambda(v, err);
}

/* NOTE: this is a deep copy
*/
lval* lval_copy_pair(lval* src, err_t* err) {
  lval* car = lval_copy(src->car, err);
  lval* dest;
  if (EMPTY_LIST == src->cdr) {
    dest = lval_pair(car, EMPTY_LIST, err);
  } else {
    lval* cdr = lval_copy(src->cdr, err);
    dest = lval_pair(car, cdr, err);
  }
  return dest;
}

/* purpose: dispatch copy function based on lval type
*/
lval* lval_copy(lval* v, err_t* err) {
  if (!v) {
    return NULL;
  }

  switch (v->type) {
    case LVAL_UNDEF:
      return lval_undef(err);
    case LVAL_NUM:
      return lval_num(v->num, err);
    case LVAL_BOOL:
      return lval_bool(v->is_true, err);
    case LVAL_PROC:
      return lval_copy_func(v, err);
    case LVAL_SYM:
      return lval_sym(v->sym, err);
    case LVAL_PAIR:
      return lval_copy_pair(v, err);
    default:
      fucked_up("lval_copy", "this type doesn't exist, yo!");
  }
  return NULL; /* should not be reached, just to shut up compiler */
}

/********************************************************************************
* READING
********************************************************************************/
/*
*/
typedef enum ast_tag_t {
  UNRECOGNIZED_TAG,
  NUMBER_TAG,
  SYMBOL_TAG,
  LIST_TAG,
  TOPLEVEL_TAG,
} ast_tag_t;
ast_tag_t tag_map(char* tag) {
  if (strstr(tag, "number")) {
    return NUMBER_TAG;
  } else if (strstr(tag, "symbol")) {
    return SYMBOL_TAG;
  } else if (strstr(tag, "list")) {
    return LIST_TAG;
  } else if (!strcmp(tag, ">")) {
    return TOPLEVEL_TAG;
  }

  return UNRECOGNIZED_TAG;
}

/*
*/
lval* l_rip(lval* pair) {
  if (EMPTY_LIST == pair) {
    return NULL;  // this is just a signal that means nothing more to return
  }

  lval* x = pair->car;
  pair->car = NULL;
  lval_del(pair);
  return x;
}

/*
*/
lval* l_pop(lval** pair) {
  if (EMPTY_LIST == (*pair)) {
    return NULL;  // this is just a signal that means nothing more to return
  }

  lval* base = *pair;
  lval* x = base->car;
  (*pair) = base->cdr;
  base->car = base->cdr = NULL;  // cut pointers
  lval_del(base);
  return x;
}

/* If read unsuccessful, will clean up ENTIRE parent tree
** ast is only read
*/
lval* read_children(lval* root, mpc_ast_t* t, err_t* err) {
  lval* x = root;
  lval* next_child;
  size_t count = t->children_num;
  for (size_t i = 0; i < count; i = inc_size(i)) {
    /* if not a bracket, then it is a child value */
    if (strcmp(t->children[i]->tag, "regex") &&
        strncmp(t->children[i]->contents, "(", 1) &&
        strncmp(t->children[i]->contents, ")", 1)) {
      next_child = read_lval(t->children[i], err);
      if (EMPTY_LIST == x) {
        /* root is now a pair, not an empty_list */
        root = lval_pair(next_child, EMPTY_LIST, err);
        x = root; /* update current spot */
      } else {
        /* append new pair to current spot*/
        x->cdr = lval_pair(next_child, EMPTY_LIST, err);
        if (err->sig) {
          exit(1);
        }
        /* update current spot */
        x = x->cdr;
      }
    }
  }
  return root;
}

/*
*/
lval* read_lval_list(mpc_ast_t* t, err_t* err) {
  lval* x = EMPTY_LIST;
  return read_children(x, t, err);
}

/*
*/
lval* lisp_read(mpc_ast_t* t, err_t* err) {
  lval* x = read_lval(t, err);  // propagate error
  mpc_ast_delete(t);            // clean up
  return x;                     // propage error
}

/* purpose: dispatch reader function based on ast tag
*/
lval* read_lval(mpc_ast_t* t, err_t* err) {
  ast_tag_t tag = tag_map(t->tag);

  switch (tag) {
    case NUMBER_TAG:
      return read_num(t, err);
    case SYMBOL_TAG:
      return lval_sym(t->contents, err);
    case LIST_TAG:
      return read_lval_list(t, err);
    case TOPLEVEL_TAG:
      return read_lval(t->children[1], err);
    default:
      printf("ERROR unrecognized tag");
      exit(1);
  }
}

/*
*/
lval* read_num(mpc_ast_t* t, err_t* err) {
  errno = 0;
  int64_t num = strtol(t->contents, NULL, 10);
  if (errno == ERANGE) {
    return lval_err("number out of range", err);
  }

  return lval_num(num, err);
}

/********************************************************************************
* PRINT
********************************************************************************/
/*
*/
void print(lval* x) {
  print_lval(x);
  lval_del(x);
}

/*
*/
void print_lval_func(lval* x) {
  if (x->builtin) {
    printf("<builtin> ");
  } else {
    printf("<lambda ");
    print_lval(x->parameters);
    print_lval(x->exp);
    printf("> ");
  }
}

/* purpose: dispatch print based on lval type
*/
void print_lval(lval* x) {
  if (EMPTY_LIST == x) {
    printf("'()");
    return;
  }

  lval* a;
  switch (x->type) {
    case LVAL_NUM:
      printf("%ld ", x->num);
      break;
    case LVAL_BOOL:
      if (x->is_true) {
        printf("#t ");
      } else {
        printf("#f ");
      }
      break;
    case LVAL_SYM:
      printf("%s ", x->sym);
      break;
    case LVAL_ERR:
      printf("[ERROR: %s] ", x->err);
      break;
    case LVAL_PAIR:
      printf("(");
      print_lval(x->car);
      for (a = x->cdr; EMPTY_LIST != a; a = a->cdr) {
        print_lval(a->car);
      }
      printf(") ");
      break;
    case LVAL_UNDEF:
      printf("<undef> ");
      break;
    case LVAL_PROC:
      print_lval_func(x);
      break;
    default:
      fucked_up("print_lval", "this type can't be printed, yo!");
      break;
  }
}

/********************************************************************************
* EVAL
********************************************************************************/
/*
*/
lval* eval_children(symbol_map* sym_env, lval* parent, err_t* err) {
  for (lval* x = parent; EMPTY_LIST != x; x = x->cdr) {
    x->car = eval_lval(sym_env, x->car, err);
  }

  return parent;
}

/*
*/
lval* eval_pair(symbol_map* sym_env, lval* args, err_t* err) {
  /* binding constructs and definitions and special procedures? */
  if (LVAL_SYM == args->car->type) {
    char* first_sym = args->car->sym;
    if (!strcmp("quote", first_sym)) {
      lval_del(l_pop(&args));  // remove id
      return builtin_quote(sym_env, args, err);
    } else if (!strcmp("if", first_sym)) {
      lval_del(l_pop(&args));  // remove id
      return builtin_if(sym_env, args, err);
    } else if (!strcmp("lambda", first_sym)) {
      lval_del(l_pop(&args));  // remove id
      return builtin_lambda(sym_env, args, err);
    } else if (!strcmp("define", first_sym)) {
      lval_del(l_pop(&args));  // remove id
      return builtin_define(sym_env, args, err);
    } else if (!strcmp("set!", first_sym)) {
      lval_del(l_pop(&args));  // remove id
      return builtin_set(sym_env, args, err);
    }
  }

  /* regular procedures */
  args = eval_children(sym_env, args, err);
  if (LVAL_ERR == args->type) {
    return args;
  }
  lval* func = l_pop(&args);

  /* make sure first arg is a func */
  if (LVAL_PROC != func->type) {
    lval_del(func);
    lval_del(args);
    return lval_err("1st element of list is not a func",
                    err); /* propagate error */
  }

  /* dispatch */
  if (func->builtin) {
    lval* x = func->builtin(sym_env, args, err);
    lval_del(func);
    return x;
  }

  return dispatch_lambda(sym_env, func, args, err);
}

/* dipsatch eval function based on lval type
*/
lval* eval_lval(symbol_map* sym_env, lval* v, err_t* err) {
  if (EMPTY_LIST == v) {
    return EMPTY_LIST;
  }

  switch (v->type) {
    case LVAL_SYM:
      // puts("sym");
      return eval_symbol(sym_env, v, err);
    case LVAL_PAIR:
      // puts("pair");
      return eval_pair(sym_env, v, err);
    case LVAL_PROC:
      // puts("fun");
      return v;
    case LVAL_NUM:
      // puts("num");
      return v;
    case LVAL_BOOL:
      // puts("bool");
      return v;
    case LVAL_UNDEF:
      // puts("undef");
      return v;
    case LVAL_ERR:
      // puts("err");
      return v;
    default:
      fucked_up("eval_lval", "I don't recognize this type yo");
      break;
  }
  return NULL; /* should not be reached, just to shut up compiler */
}

/*
*/
lval* dispatch_lambda(symbol_map* sym_map, lval* func, lval* args, err_t* err) {
  lval* params = func->parameters;
  lval* expression = func->exp;
  func->exp = func->parameters = NULL;
  lval_del(func);

  /* symbol env */
  // TODO I am just overwriting sym_map here, why do I need to pass it at all?
  sym_map = populate(SYM_MAP, params, args, err);
  if (err->sig) {
    return lval_clean(expression, NULL, NULL);
  }  // couldn't populate

  /* evaluate */
  lval* return_val = eval_pair(sym_map, expression, err);
  if (err->sig) {
    return NULL;
  }

  /* clean up */
  sym_map_del(sym_map);
  return return_val;
}

/*******************************************************************************
* SYMBOL UTILS
*******************************************************************************/
/* Does not modify anything, merely returns a pointer
*/
binding_t* sym_search(symbol_map* env, char* key) {
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

//======================================
lval* builtin_define(symbol_map* sym_env, lval* args, err_t* err) {
  // TODO check preconditions

  /* set up pointers */
  /*
  if( 2 != args->count ) {
    return lval_err("define accepts exactly 2 args", err);
  }
  */

  lval* one = l_pop(&args);
  if (!one) {
    lval_del(args);
    return lval_err("DEFINE expects 2 args, recieved none", err);
  }

  lval* x = l_pop(&args);
  if (!x) {
    lval_clean(args, one, NULL);
    return lval_err("DEFINE expects 2 args, received only one", err);
  }

  /* rip symbol */
  if (LVAL_SYM != one->type) {
    return lval_err("eval expects 1st arg to be of type LVAL_SYM", err);
  }
  char* symbol = rip_sym(one);

  /* eval */
  x = eval_lval(sym_env, x, err);
  if (err->sig) {
    return lval_clean(x, NULL, NULL);
  } /* couldn't eval def target */

  /* push */
  bool err_x = symbol_add(SYM_MAP, symbol, x,
                          err);  // note: adding directly to global sym_map
  if (err_x) {
    return lval_err("symbol already exists", err);
  }
  if (err->sig) { /* werent able to make space in the sym_map on top of
                   * sym_env
                     */
    // try to shrink sym_map back down
    // success -> then clean up and pop back
    // error -> fatal error
    printf("TODO global symbol stack error");
    exit(1);
  }

  return lval_undef(err);
}

//======================================
// FUNCTION: what you want to return is already in a list, just label it a
// qexp
lval* builtin_list(symbol_map* sym_env, lval* args, err_t* err) {
  if (EMPTY_LIST == args) {
    return EMPTY_LIST;
  }

  args->quoted = true;
  return args;
}

//======================================
lval* builtin_head(symbol_map* sym_env, lval* args, err_t* err) {
  // check inputs
  /*
  if( 1 != args->count ) {
    return lval_err("head accepts exactly 1 arg", err);
  }
  */

  // TODO check arg is sexp?

  lval* list = l_rip(args);
  /*
  if( list->count < 1 ) {
    return lval_err("head requires a list of length at least 1", err);
  }
  */

  // by this point: x is qexp of size > 1
  lval* head = l_rip(list);

  // preserve quoting for nested expressions
  head->quoted = true;  // TODO is this right?

  return head;
}

//======================================
lval* builtin_tail(symbol_map* sym_env, lval* x, err_t* err) {
  /*
  //check good state
  if( 1 != args->count ) {
    return lval_err("head accepts exactly 1 arg", err);
  }

  lval* arg1 = args->cell[0];
  if( !(arg1->count) ) {
    return lval_err("tail requires a list of length at least 1", err);
  }
  */

  x = l_rip(x);        /* rip first arg and free the rest */
  lval_del(l_pop(&x)); /* pop and discard the head */
  return x;            /* return the tail */
}

lval* builtin_join(symbol_map* sym_env, lval* args, err_t* err) {
  return NULL;
}

//======================================
lval* builtin_eval(symbol_map* sym_env, lval* args, err_t* err) {
  return NULL;
  //##  /*
  //##  if( 1 != args->count ) {
  //##    return lval_err("eval accepts exactly 1 arg", err);
  //##  }
  //##  */

  //##  lval* input = lval_rip(args); // the rest of ARGS will be freed here
  //##  // TODO input = lval_pop(args); if( !sexp_empty(args) ){ lval_err(....
  //##  if (!(input->quoted)) {
  //##    return lval_err("eval expected qexp", err);
  //##  }

  //##  input->type = LVAL_SEXP;
  //##  return eval_lval(sym_env, input, err);
}

/* Note checking for overflow should be the job of LISP not here at a low
 * level
*/
lval* proc_sum(symbol_map* sym_env, lval* args, err_t* err) {
  lval* x = l_pop(&args);
  for (lval* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num += y->num;

    lval_del(y);
  }

  lval_del(args);
  return x;
}

/*
*/
lval* proc_minus(symbol_map* sym_env, lval* args, err_t* err) {
  lval* x = l_pop(&args);
  for (lval* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num -= y->num;

    lval_del(y);
  }

  lval_del(args);
  return x;
}

/*
*/
lval* proc_prod(symbol_map* sym_env, lval* args, err_t* err) {
  lval* x = l_pop(&args);
  for (lval* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num *= y->num;

    lval_del(y);
  }

  lval_del(args);
  return x;
}

/*
*/
lval* proc_quotient(symbol_map* sym_env, lval* args, err_t* err) {
  lval* x = l_pop(&args);
  for (lval* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num /= y->num;

    lval_del(y);
  }

  lval_del(args);
  return x;
}

/*
*/
lval* builtin_lambda(symbol_map* sym_env, lval* args, err_t* err) {
  lval* parameters = l_pop(&args);
  lval* exp = l_pop(&args);
  // TODO again, use lval_empty to check for preconditions
  // this will also involve making sure parameters and exp are not NULL
  lval_del(args);
  return lval_lambda(parameters, exp, err);
}

/* look for symbol, if value found, return a copy
*/
lval* eval_symbol(symbol_map* sym_env, lval* x, err_t* err) {
  char* sym = rip_sym(x);

  binding_t* binding = sym_search(sym_env, sym);
  if (!binding) {
    char* err_msg = malloc(
        (strlen("symbol [] is not bound") + 1 + strlen(sym)) * sizeof(char));
    sprintf(err_msg, "symbol [%s] is not bound", sym);
    free(sym);
    lval* a = lval_err(err_msg, err);
    free(err_msg);
    return a;
  }
  free(sym);

  /* return a copy of the value */
  /* TODO it should actually return a pointer to the value directly */
  return lval_copy(binding->value, err);
}

/********************************************************************************
* SYMBOL UTILS
********************************************************************************/
/*
*/
char* builtin_names[] = {"eval",    "list",    "car",        "boolean?",
                         "number?", "symbol?", "procedure?", "pair?",
                         "eqv?",    "+",       "-",          "*",
                         "/",       "quote",   NULL};

/* dispatch builtin function based on string
*/
builtin_fun* proc_tab(char* x) {
  if (!strcmp("define", x)) {
    return builtin_define;
  } else if (!strcmp("quote", x)) {
    return builtin_quote;
  } else if (!strcmp("boolean?", x)) {
    return proc_pred_bool;
  } else if (!strcmp("number?", x)) {
    return proc_pred_num;
  } else if (!strcmp("symbol?", x)) {
    return proc_pred_sym;
  } else if (!strcmp("procedure?", x)) {
    return proc_pred_fun;
  } else if (!strcmp("pair?", x)) {
    return proc_pred_pair;
  } else if (!strcmp("eqv?", x)) {
    return proc_pred_eqv;
  } else if (!strcmp("car", x)) {
    return proc_car;
  } else if (!strcmp("lambda", x)) {
    return builtin_lambda;
  } else if (!strcmp("eval", x)) {
    return builtin_eval;
  } else if (!strcmp("list", x)) {
    return builtin_list;
  } else if (!strcmp("+", x)) {
    return proc_sum;
  } else if (!strcmp("-", x)) {
    return proc_minus;
  } else if (!strcmp("*", x)) {
    return proc_prod;
  } else if (!strcmp("/", x)) {
    return proc_quotient;
  }

  fucked_up("proc_tab", "unrecognized string");
  return 0;
}

/*
*/
symbol_map* init_global_map(err_t* err) {
  symbol_map* sym_map = sym_map_make(err);
  if (err->sig) {
    return NULL;
  }

  char* x;
  for (size_t i = 0; (x = builtin_names[i]); i = inc_size(i)) {
    push_builtin(sym_map, x, err);
    if (err->sig) {
      sym_map_del(sym_map);
      return NULL;
    }
  }

  symbol_add(sym_map, strdup("#t", err), lval_bool(true, err), err);
  symbol_add(sym_map, strdup("#f", err), lval_bool(false, err), err);

  return sym_map;
}

/*
*/
void push_builtin(symbol_map* sym_map, char* fun_name, err_t* err) {
  char* funx = strdup(fun_name, err);
  if (err->sig) {
    free(funx);
    return;
  }

  lval* xx = lval_builtin(proc_tab(fun_name), err);
  if (err->sig) {
    free(funx);
    lval_del(xx);
    return;
  }

  symbol_add(sym_map, funx, xx, err);  // PPG ERR
}

/* note: this is the correct interface function to use (not `kv_push`)
** consumes: symbol, value
*/
bool symbol_add(symbol_map* sym_map, char* symbol, lval* value, err_t* err) {
  // make sure there is something in the symbol_map before you being searching
  if (sym_map && sym_search(sym_map, symbol)) {
    free(symbol);
    lval_del(value);
    return true;
  }

  kv_push(sym_map, symbol, value, err);
  return false;
}

/* note: this will delete kv-chain but not parent
*/
void sym_map_del(symbol_map* sym_map) {
  binding_t *a, *x = sym_map->first_child;
  while (x) {
    a = x->sibling;
    free(x->key);
    lval_del(x->value);
    free(x);
    x = a;
  }

  free(sym_map);
}

/*
*/
symbol_map* sym_map_make(err_t* err) {
  symbol_map* sym_map = malloc(sizeof(symbol_map));
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
symbol_map* populate_rest(symbol_map* sym_map,
                          lval* parameters,
                          lval* args,
                          err_t* err) {
  char* sym = rip_sym(l_rip(parameters));
  args->quoted = true;  // TODO should this be higher up the chain?

  kv_push(sym_map, sym, args, err);
  if (err->sig) {
    puts("pop sym env");
    exit(1);
  }
  return sym_map;
}

/* WARNING: this function assumes:
** * parameters->count == args->count
** * parameters is a sexp with only LVAL_SYM as children
** * args is a sexp
** ---
** consumes parameters
** consumes args
*/
symbol_map* populate(symbol_map* parent,
                     lval* parameters,
                     lval* args,
                     err_t* err) {
  /* create blank slate; */
  symbol_map* sym_map = sym_map_make(err);
  if (err->sig) {
    lval_clean(args, parameters, NULL);
    return NULL;
  }
  sym_map->parent = parent;

  /* populate */
  char* sym;
  for (lval* a = l_pop(&parameters); a; a = l_pop(&parameters)) {
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
  lval_clean(args, parameters, NULL);
  return sym_map;
}

/* consumes key
** consumes value
** modifies sym_map
** modifies error
*/
void kv_push(symbol_map* sym_map, char* key, lval* value, err_t* err) {
  /* allocate space */
  binding_t* kv = malloc(sizeof(binding_t));
  if (!kv) {
    free(key);
    lval_del(value);
    err->sig = OUT_OF_MEM;
    return;
  }

  /* set values */
  kv->key = key;
  kv->value = value;
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

/******************************************************************************
* GRAVEYARD
******************************************************************************/
char* lval_type_string(lval_type_t type) {
  switch (type) {
    case LVAL_NUM:
      return "num";
    case LVAL_PROC:
      return "function";
    case LVAL_SYM:
      return "sym";
    case LVAL_BOOL:
      return "bool";
    case LVAL_ERR:
      return "err";
    case LVAL_UNDEF:
      return "undef";
    case LVAL_PAIR:
      return "pair";
  }

  return "UNKNOWN TYPE";
}

/*******************************************************************************
* New
*******************************************************************************/
lval* builtin_quote(symbol_map* sym_map, lval* args, err_t* err) {
  lval* x = l_pop(&args);  // WARNING not error checking
  return x;
}

/*
*/
bool scheme_true(lval* x) {
  if (LVAL_BOOL == x->type) {
    return x->is_true;
  }

  return true;
}

/*
*/
lval* builtin_if(symbol_map* sym_map, lval* args, err_t* err) {
  lval* cond = l_pop(&args);
  lval* a = l_pop(&args);
  lval* b = l_pop(&args);
  lval_del(args);

  cond = eval_lval(sym_map, cond, err);
  if (scheme_true(cond)) {
    lval_clean(cond, b, NULL);
    return eval_lval(sym_map, a, err);
  } else {
    lval_clean(cond, a, NULL);
    if (b) {
      return eval_lval(sym_map, b, err);
    } else {
      return lval_undef(err);
    }
  }
}

/*******************************************************************************
* PREDICATES
*******************************************************************************/
lval* scheme_type_predicate(lval_type_t target, lval* x, err_t* err) {
  if (target == rip_type(x)) {
    return lval_bool(true, err);
  }
  return lval_bool(false, err);
}

lval* scm_rip_one_arg(lval* args) {
  lval* x = l_pop(&args);
  if (EMPTY_LIST != args) {
    puts("only one arg expected!");
    exit(1);
  }

  return x;
}

lval* proc_pred_bool(symbol_map* sym_map, lval* args, err_t* err) {
  lval* x = scm_rip_one_arg(args);
  if (EMPTY_LIST == x) {
    return lval_bool(false, err);
  }
  return scheme_type_predicate(LVAL_BOOL, x, err);
}

lval* proc_pred_num(symbol_map* sym_map, lval* args, err_t* err) {
  lval* x = scm_rip_one_arg(args);
  if (EMPTY_LIST == x) {
    return lval_bool(false, err);
  }
  return scheme_type_predicate(LVAL_NUM, x, err);
}

lval* proc_pred_sym(symbol_map* sym_map, lval* args, err_t* err) {
  lval* x = scm_rip_one_arg(args);
  if (EMPTY_LIST == x) {
    return lval_bool(false, err);
  }
  return scheme_type_predicate(LVAL_SYM, x, err);
}

lval* proc_pred_fun(symbol_map* sym_map, lval* args, err_t* err) {
  lval* x = scm_rip_one_arg(args);
  if (EMPTY_LIST == x) {
    return lval_bool(false, err);
  }
  return scheme_type_predicate(LVAL_PROC, x, err);
}

lval* proc_pred_pair(symbol_map* sym_map, lval* args, err_t* err) {
  lval* x = scm_rip_one_arg(args);
  if (EMPTY_LIST == x) {
    return lval_bool(false, err);
  }
  return scheme_type_predicate(LVAL_PAIR, x, err);
}

lval* proc_pred_eqv(symbol_map* sym_map, lval* args, err_t* err) {
  lval* a = l_pop(&args);
  lval* b = l_pop(&args);
  lval_del(args);

  /* deal with empty list */
  if ((EMPTY_LIST == a) && (EMPTY_LIST == b)) {
    return lval_bool(true, err);
  }

  /* if not same type, can't be  eqv */
  if (a->type != b->type) {
    lval_clean(a, b, NULL);
    return lval_bool(false, err);
  }

  bool x = false;
  switch (a->type) {
    case LVAL_BOOL:
      x = (a->is_true == b->is_true);
      break;
    case LVAL_SYM:
      x = !strcmp(a->sym, b->sym);
      break;
    case LVAL_NUM:
      x = (a->num == b->num);
      break;
    case LVAL_PAIR:
      x = (a == b);
      break;
    case LVAL_PROC:
      x = (a->builtin == b->builtin);
      break;
    default:
      fucked_up("proc_pred_eqv", "type is not recognized yo");
  }

  lval_clean(a, b, NULL);
  return lval_bool(x, err);
}

/*
*/
void scm_expect_type(lval_type_t type, lval* x) {
  if (EMPTY_LIST == x) {
    fucked_up("scm_expect_type", "empty list");
  }

  if (type != x->type) {
    printf("expected: %s\n", lval_type_string(type));
    printf("instead got: %s\n", lval_type_string(x->type));
    fucked_up("scm_expect_type", "type is not as dictated");
  }
}

/* TODO fix the memory semantics of this, something else has to delete the cdr
 * of the list
*/
lval* proc_car(symbol_map* sym_env, lval* args, err_t* err) {
  lval* x = scm_rip_one_arg(args);
  scm_expect_type(LVAL_PAIR, x);

  return x->car;
}

/*
*/
lval* builtin_set(symbol_map* sym_env, lval* args, err_t* err) {
  /* set args */
  lval* variable = l_pop(&args);
  lval* expression = l_pop(&args);
  lval_del(args);

  /* search for binding */
  char* sym = rip_sym(variable);
  binding_t* binding = sym_search(sym_env, sym);
  if (!binding) {
    char* err_msg = malloc((strlen("symbol [] is unbound") + 1 + strlen(sym)) *
                           sizeof(char));
    sprintf(err_msg, "symbol [%s] is unbound", sym);
    free(sym);
    lval* a = lval_err(err_msg, err);
    free(err_msg);
    return a;
  }
  free(sym);

  /* evaluate expression and set new value */
  lval* value = eval_lval(sym_env, expression, err);
  lval_del(binding->value);
  binding->value = value;

  return lval_undef(err);
}
