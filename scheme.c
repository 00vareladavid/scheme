/* Scheme implementation
*/
#include "linenoise/linenoise.h"
#include "mpc/mpc.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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
struct sym_env_t;
typedef struct sym_env_t sym_env_t;
struct err_t;
typedef struct err_t err_t;
struct lval_t;
typedef struct lval_t lval_t;

typedef lval_t*(builtin_fun)(sym_env_t*, lval_t*, err_t*);

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

/********************************************************************************
* PROTOTYPES
********************************************************************************/
static unsigned char* read_whole_file(const char* file_name);
void scheme_stdlib(sym_env_t* sym_env, err_t* err);

// parser
lval_t* my_parse(err_t* err);

// debugging
char* lval_type_string(lval_type_t);

// main
char* prompt(void);
void repl(sym_env_t* sym_env, err_t* err);

// constructors
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

// lval utils
char* rip_sym(lval_t* v);
bool scheme_assert_type(lval_type_t target, lval_t* x);

// lval del
void lval_del(lval_t* v);
lval_t* lval_clean(lval_t* a, lval_t* b);

// lval copy
lval_t* lval_copy(lval_t* v, err_t* err);

// lval read
lval_t* lisp_read(mpc_ast_t* t, err_t* err);
lval_t* read_lval(mpc_ast_t* t, err_t* err);
lval_t* read_children(lval_t* parent, mpc_ast_t* t, err_t* err);
lval_t* read_num(mpc_ast_t* t, err_t* err);

// lval print
void print_lval(lval_t* x);

// lval eval
lval_t* eval_lval(sym_env_t*, lval_t* v, err_t* err);
lval_t* eval_symbol(sym_env_t* sym_env, lval_t* x, err_t* err);
lval_t* dispatch_lambda(sym_env_t*, lval_t* func, lval_t* args, err_t* err);

/* BUILTINS */

/* most complex builtins, require all three args */
lval_t* builtin_define(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* proc_begin(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* builtin_set(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* builtin_eval(sym_env_t* sym_env, lval_t* args, err_t* err);

/* these builtins operate directly on their args, no sym_env needed */
lval_t* builtin_lambda(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* builtin_head(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* builtin_tail(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* builtin_join(sym_env_t* sym_env, lval_t* args, err_t* err);

/* safe builtins, no chance of memory errors */
lval_t* builtin_quote(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* builtin_if(sym_env_t* sym_map, lval_t* args, err_t* err);

lval_t* proc_pred_bool(sym_env_t* sym_map, lval_t* args, err_t* err);
lval_t* proc_pred_num(sym_env_t* sym_map, lval_t* args, err_t* err);
lval_t* proc_pred_sym(sym_env_t* sym_map, lval_t* args, err_t* err);
lval_t* proc_pred_fun(sym_env_t* sym_map, lval_t* args, err_t* err);
lval_t* proc_pred_pair(sym_env_t* sym_map, lval_t* args, err_t* err);

lval_t* proc_pred_eqv(sym_env_t* sym_map, lval_t* args, err_t* err);

lval_t* proc_car(sym_env_t* sym_map, lval_t* args, err_t* err);
lval_t* proc_list(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* proc_sum(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* proc_minus(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* proc_prod(sym_env_t* sym_env, lval_t* args, err_t* err);
lval_t* proc_quotient(sym_env_t* sym_env, lval_t* args, err_t* err);

/* symbols */
sym_env_t* init_global_map(err_t* err);
void push_builtin(sym_env_t* sym_map, char* fun_name, err_t* err);
bool symbol_add(sym_env_t* sym_map, char* symbol, lval_t* value, err_t* err);
void kv_push(sym_env_t* sym_map, char* key, lval_t* value, err_t* err);
sym_env_t* sym_map_make(err_t* err);
void sym_map_del(sym_env_t* sym_map);
builtin_fun* proc_tab(char* x);
binding_t* sym_search(sym_env_t* env, char* key);
sym_env_t* populate(sym_env_t* parent,
                    lval_t* parameters,
                    lval_t* args,
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
sym_env_t* SYM_MAP;
static lval_t* NIL;
static lval_t* L_T;
static lval_t* L_F;

/******************
* GC
******************/
// TODO make sure NIL is never freed
// TODO make sure that refCount does not overflow!
lval_t* incRef(lval_t* x) {
  switch (x->type) {
    case LVAL_PAIR:
      incRef(x->car);
      incRef(x->cdr);
    case LVAL_NUM:
    case LVAL_SYM:
    case LVAL_UNDEF:
    case LVAL_ERR:
    case LVAL_PROC:
      x->refCount++;
    case LVAL_SPECIAL:
    case LVAL_BOOL:
      break;
  }

  printf("incRef: {");
  print_lval(x);
  printf("| %d }\n", x->refCount);
  return x;
}

/*
*/
void gc_attempt(lval_t* x) {
  switch (x->type) {
    case LVAL_PAIR:
    case LVAL_NUM:
    case LVAL_SYM:
    case LVAL_UNDEF:
    case LVAL_ERR:
    case LVAL_PROC:
      if (!(x->refCount)) {
        lval_del(x);
      }
    /* special object for which it is only necessary to allocate a single global
     * object */
    case LVAL_SPECIAL:
    case LVAL_BOOL:
      break;
  }
}

/* This will first reduce the reference count for each object
 * Then it will free any objects which are part of that structure
*/
void decRef(lval_t* x) {
  if (!x) {
    return;
  }

  /*
  printf("decRef: {");
  print_lval(x);
  printf("}\n");
  */
  switch (x->type) {
    case LVAL_PAIR:
      decRef(x->car);
      decRef(x->cdr);
    case LVAL_NUM:
    case LVAL_SYM:
    case LVAL_UNDEF:
    case LVAL_ERR:
    case LVAL_PROC:
      x->refCount--;
      gc_attempt(x);
    case LVAL_SPECIAL:
    case LVAL_BOOL:
      break;
  }
}

/******************
* PARSER
******************/
char* INPUT;

bool whitespace_char(char x) {
  return (x == ' ') || (x == '\n');
}

bool delimiting_char(char x) {
  return (x == ' ') || (x == '\n') || (x == '(') || (x == ')') || (x == '\0');
}

/* skip whitespace and comments
*/
void skip_trash(void) {
  /* skip whitespace */
  while (*INPUT && whitespace_char(*INPUT)) {
    ++INPUT;
  }

  /* skip comments */
  if (';' == *INPUT) {
    ++INPUT;
    while (*INPUT && ('\n' != *INPUT)) {
      ++INPUT;
    }
    skip_trash();
  }
}

/*
*/
lval_t* parse_bool(char* tok, err_t* err) {
  lval_t* x;
  if ('t' == tok[0]) {
    x = get_bool(true);
  } else if ('f' == tok[0]) {
    x = get_bool(false);
  } else {
    puts("1 unknown reader macro");
    exit(1);
  }

  if ((tok + 1) != INPUT) {
    puts("2 unknown reader macro");
    exit(1);
  }

  return x;
}

lval_t* parse_sym(char* tok, err_t* err) {
  size_t diff = INPUT - tok;
  char* x = malloc(sizeof(char) * (diff + 1));
  memcpy(x, tok, diff);
  x[diff] = '\0';
  lval_t* v = lval_sym(x, err);
  free(x);
  return v;
}

lval_t* parse_num(char* tok, err_t* err) {
  errno = 0;
  char* end;
  int64_t num = strtol(tok, &end, 10);
  if (errno == ERANGE) {
    return lval_err("number out of range", NULL);
  }

  return lval_num(num, err);
}

lval_t* parse_list(err_t* err) {
  lval_t* root = NIL;
  lval_t* x = NULL;
  for (lval_t* next_child = my_parse(err); next_child;
       next_child = my_parse(err)) {
    if (NIL == root) {
      /* root is now a pair, not an empty_list */
      root = lval_pair(next_child, NIL, err);
      x = root; /* update current spot */
    } else {
      /* append new pair to current spot*/
      x->cdr = lval_pair(next_child, NIL, err);
      /* update current spot */
      x = x->cdr;
    }
  }

  return root;
}

lval_t* parse_token(char* tok, err_t* err) {
  switch (tok[0]) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return parse_num(tok, err);
    case '+':
    case '-':
      if (INPUT == tok + 1) {
        return parse_sym(tok, err);
      } else {
        return parse_num(tok, err);
      }
    case '#':
      return parse_bool(tok + 1, err);
    case '(':
      return parse_list(err);
    case ')':
      return NULL;
    default: /* has to be an identfier */
      return parse_sym(tok, err);
  }

  return NULL;  // just for the compiler
}

void delimit_tok(char* tok) {
  if ('\0' == *tok) {
    return;
  }

  /* single char tokens will be delimited by anything */
  if ('(' == *tok || ')' == *tok) {
    ++INPUT;
    return;
  }

  /* otherwise */
  ++INPUT;
  while (!delimiting_char(INPUT[0])) {
    ++INPUT;
  }
}

/* tok starts token and INPUT is first delimiting char
*/
lval_t* my_parse(err_t* err) {
  // printf("\n\nleft is: *%s*\n\n", INPUT);

  /* skip whitespace and comments */
  skip_trash();

  /* trash till end means no more input */
  if ('\0' == INPUT[0]) {
    return NULL;
  }

  /* INPUT will skip to first delimiting char */
  char* tok = INPUT;
  delimit_tok(tok);

  /* parse into an lval_t* */
  return parse_token(tok, err);
}

/*
*/
lval_t* ll_push(lval_t* x, lval_t* xs) {
  err_t* dummy = malloc(sizeof(err_t));
  dummy->sig = 0;

  lval_t* base = lval_pair(x, xs, dummy);
  free(dummy);
  return base;
}

/*
*/
lval_t* ll_reverse(lval_t* xs) {
  if (NIL == xs) {
    return NIL;
  }

  /* chop */
  lval_t* old_base = xs;
  xs = xs->cdr;

  /* special case */
  old_base->cdr = NIL;

  lval_t* base;
  while (NIL != xs) {
    /* chop */
    base = xs;
    xs = xs->cdr;

    /* append */
    base->cdr = old_base;
    old_base = base;
  }

  return old_base;
}

/*
*/
lval_t* start_parse(char* input, err_t* err) {
  INPUT = "(begin)";
  lval_t* all = my_parse(err);

  INPUT = input;
  lval_t* x = my_parse(err);
  while (x) {
    all = ll_push(x, all);
    x = my_parse(err);
  }
  all = ll_reverse(all);
  return all;
}

/*
*/
lval_t* single_parse(char* input, err_t* err) {
  INPUT = input;
  lval_t* x = my_parse(err);
  free(input);
  return x;
}

/********************************************************************************
* MAIN
********************************************************************************/
int main(int argc, char* argv[]) {
  /* error struct */
  err_t* err = malloc(sizeof(err_t));
  if (!err) {
    printf("1 insufficient mem for baseline framework\n");
    exit(1);
  }
  err->sig = OK;

  /* SPECIAL VALUES */
  NIL = lval_special(err);
  L_T = lval_bool(true, err);
  L_F = lval_bool(false, err);

  /* symbol env */
  SYM_MAP = init_global_map(err);
  if (err->sig) {
    free(err);
    printf("2 insufficient mem for baseline framework\n");
    exit(1);
  }
  sym_env_t* sym_env = SYM_MAP;  // dirty hack to satisfy type system for now

  /*** LOAD STDLIB ***/
  scheme_stdlib(sym_env, err);

  /*** REPL ***/
  repl(sym_env, err);

  /*** CLEAN ***/
  sym_map_del(SYM_MAP);
  free(err);
  free(NIL);
  free(L_T);
  free(L_F);
  return 0;
}

/*******************************************************************************
* Standard Library
*******************************************************************************/
lval_t* lisp_exec(sym_env_t* sym_env, char* input, err_t* err) {
  /* read */
  lval_t* x = single_parse(input, err);
  // TODO check for bad parse
  if (err->sig) {
    printf("[ERR] Unable to READ lval_t due to insufficient memory\n");
    return lval_clean(x, NULL);
  }

  /* eval */
  x = eval_lval(sym_env, x, err);
  if (err->sig) {
    printf("[ERR] Unable to EVAL lval due to insufficient memory\n");
    return lval_clean(x, NULL);
  }

  return x;
}

void scheme_stdlib(sym_env_t* sym_env, err_t* err) {
  printf("reading file ....");  // debug
  char* code = (char*)read_whole_file("std_defs.lisp");
  printf("done\n");               // debug
  printf("starting parse ....");  // debug
  lval_t* x = start_parse(code, err);
  printf("done\n");  // debug
  x = eval_lval(sym_env, x, err);
  decRef(x);
  free(code);
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
void repl(sym_env_t* sym_env, err_t* err) {
  puts("Scheme v0.1.0");
  puts("Enter 'EXIT' to exit");

  lval_t* x;
  char* input = prompt();
  while (strcmp(input, "QUIT")) {
    x = lisp_exec(sym_env, input, err);
    /* if error: reset signal and skip printing */
    if (err->sig) {  // TODO I am throwing all kinds of errors together,
                     // seperate them
      err->sig = OK;
    } else {
      print_lval(x);
      decRef(x);
    }
    input = prompt();
  }
  free(input);
}

/********************************************************************************
* CONSTRUCTORS
********************************************************************************/
lval_t* new_lval(lval_type_t type, err_t* err) {
  lval_t* v = malloc(sizeof(lval_t));
  if (!v) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = type;
  v->refCount = 1;
  return v;
}

lval_t* lval_pair(lval_t* car, lval_t* cdr, err_t* err) {
  lval_t* v = new_lval(LVAL_PAIR, err);
  if (err->sig) {
    return lval_clean(car, cdr);
  }

  v->car = car;
  v->cdr = cdr;
  return v;
}

lval_t* lval_special(err_t* err) {
  lval_t* v = new_lval(LVAL_SPECIAL, err);
  if (err->sig) {
    return NULL;
  }

  return v;
}

lval_t* lval_num(int64_t number, err_t* err) {
  lval_t* v = new_lval(LVAL_NUM, err);
  if (err->sig) {
    return NULL;
  }

  v->num = number;
  return v;
}

/*
*/
lval_t* lval_bool(bool x, err_t* err) {
  lval_t* v = new_lval(LVAL_BOOL, err);
  if (err->sig) {
    return NULL;
  }

  v->is_true = x;
  return v;
}

/*
*/
lval_t* get_bool(bool x) {
  if (x) {
    return L_T;
  }

  return L_F;
}

/*
*/
lval_t* lval_err(char* err_msg, err_t* err) {
  lval_t* v = new_lval(LVAL_ERR, err);
  if (err->sig) {
    return NULL;
  }

  v->err = strdup(err_msg, err);
  if (err->sig) {
    return lval_clean(v, NULL);
  }
  return v;
}

/* sym_string is only read
*/
lval_t* lval_sym(char* sym_string, err_t* err) {
  lval_t* v = new_lval(LVAL_SYM, err);
  if (err->sig) {
    return NULL;
  }

  v->identifier = strdup(sym_string, err);
  if (err->sig) {
    return lval_clean(v, NULL);
  }
  return v;
}

/*
*/
lval_t* lval_builtin(builtin_fun* fun, err_t* err) {
  lval_t* v = new_lval(LVAL_PROC, err);
  if (err->sig) {
    return NULL;
  }

  v->builtin = fun;
  return v;
}

/*
*/
lval_t* lval_undef(err_t* err) {
  lval_t* v = new_lval(LVAL_UNDEF, err);
  if (err->sig) {
    return NULL;
  }
  return v;
}

/*
*/
lval_t* lval_lambda(lval_t* parameters, lval_t* exp, err_t* err) {
  lval_t* v = new_lval(LVAL_PROC, err);
  if (err->sig) {
    return lval_clean(parameters, exp);
  }

  v->builtin = NULL;
  v->parameters = parameters;
  v->exp = exp;
  v->exp->quoted = false;  // WARNING removing the quoting
  return v;
}

/********************************************************************************
* LVAL UTILS
********************************************************************************/
/*
*/
char* rip_sym(lval_t* v) {
  char* x = strdup(v->identifier, NULL);  // TODO fix this
  decRef(v);
  return x;
}

/*
*/
lval_type_t rip_type(lval_t* v) {
  lval_type_t x = v->type;
  decRef(v);
  return x;
}

/*******************************************************************************
* DELETE
*******************************************************************************/
/* WARNING: this does not delete the struct itself, only the approriate elements
*/
void lval_del_func(lval_t* v) {
  /* if a lambda expression */
  if (!(v->builtin)) {
    decRef(v->exp);
    decRef(v->parameters);
  }

  /* nothing to delete for builtin functions */
}

/* deletes the immediate lval structure
 * WARNING: does no semantic checking
 * WARNING: does not act recursively
*/
void lval_del(lval_t* v) {
  /* no-op on NULL */
  if (!v) {
    return;
  }

  switch (v->type) {
    case LVAL_NUM:
    case LVAL_BOOL:
    case LVAL_SPECIAL:
    case LVAL_UNDEF:
    case LVAL_PAIR:
      break;
    case LVAL_ERR:
      free(v->err);
      break;
    case LVAL_SYM:
      free(v->identifier);
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
lval_t* lval_clean(lval_t* a, lval_t* b) {
  decRef(a);
  decRef(b);
  return NULL;
}

/*******************************************************************************
* COPY
*******************************************************************************/

/* copy a lambda expression
*/
lval_t* copy_lambda(lval_t* src, err_t* err) {
  lval_t* a = lval_copy(src->parameters, err);
  if (err->sig) {
    return NULL;
  }
  lval_t* b = lval_copy(src->exp, err);
  if (err->sig) {
    return lval_clean(a, NULL);
  }
  lval_t* x = lval_lambda(a, b, err);
  if (err->sig) {
    return lval_clean(a, b);
  }
  return x;
}

/*
*/
lval_t* lval_copy_func(lval_t* v, err_t* err) {
  if (v->builtin) {
    return lval_builtin(v->builtin, err);
  }

  return copy_lambda(v, err);
}

/* NOTE: this is a deep copy
*/
lval_t* lval_copy_pair(lval_t* src, err_t* err) {
  lval_t* car = lval_copy(src->car, err);
  lval_t* dest;
  if (NIL == src->cdr) {
    dest = lval_pair(car, NIL, err);
  } else {
    lval_t* cdr = lval_copy(src->cdr, err);
    dest = lval_pair(car, cdr, err);
  }
  return dest;
}

/* purpose: dispatch copy function based on lval type
*/
lval_t* lval_copy(lval_t* v, err_t* err) {
  if (!v) {
    return NULL;
  }

  switch (v->type) {
    case LVAL_UNDEF:
      return lval_undef(err);
    case LVAL_NUM:
      return lval_num(v->num, err);
    case LVAL_BOOL:
      return v;
    case LVAL_PROC:
      return lval_copy_func(v, err);
    case LVAL_SYM:
      return lval_sym(v->identifier, err);
    case LVAL_PAIR:
      return lval_copy_pair(v, err);
    default:
      fucked_up("lval_copy", "this type doesn't exist, yo!");
  }
  return NULL; /* should not be reached, just to shut up compiler */
}

/********************************************************************************
* UTILS
********************************************************************************/
/*
*/
lval_t* lval_rip(lval_t* pair) {
  if (NIL == pair) {
    return NULL;  // this is just a signal that means nothing more to return
  }

  lval_t* x = pair->car;
  pair->car = NIL;
  decRef(pair);
  return x;
}

/*
 * Conceptually, after all operations are done, only `base` gets a refCount
 * difference of -1
 * during a pop operation.
 * Coding it this way makes sure a pop stays O(1) instead of O(n)
*/
lval_t* l_pop(lval_t** pair) {
  if (NIL == (*pair)) {
    return NULL;  // this is just a signal that means nothing more to return
  }

  lval_t* base = *pair;
  lval_t* x = base->car;
  (*pair) = base->cdr;
  base->car = base->cdr = NIL;  // cut pointers
  decRef(base);
  return x;
}

/********************************************************************************
* PRINT
********************************************************************************/
/*
*/
void print_lval_func(lval_t* x) {
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
void print_lval(lval_t* x) {
  /* TODO this is only here so I don't get segmentation faults right now */
  if (!x) {
    return;
  }

  // printf("[%d]", x->refCount);//debug
  lval_t* a;
  switch (x->type) {
    case LVAL_NUM:
      // printf("<num>");//debug
      printf(" %ld", x->num);
      break;
    case LVAL_BOOL:
      // printf("<bool>");//debug
      if (x->is_true) {
        printf(" #t");
      } else {
        printf(" #f");
      }
      break;
    case LVAL_SYM:
      // printf("<sym>");//debug
      printf(" %s", x->identifier);
      break;
    case LVAL_ERR:
      // printf("<err>");//debug
      printf("[ERROR: %s] ", x->err);
      break;
    case LVAL_PAIR:
      // printf("<pair>");//debug
      printf("(");
      print_lval(x->car);
      for (a = x->cdr; NIL != a; a = a->cdr) {
        print_lval(a->car);
      }
      printf(")");
      break;
    case LVAL_UNDEF:
      // printf("<undef>");//debug
      printf("<undef>");
      break;
    case LVAL_PROC:
      // printf("<proc>");//debug
      print_lval_func(x);
      break;
    case LVAL_SPECIAL:
      // printf("<special>");//debug
      printf("'()");
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
lval_t* eval_children(sym_env_t* sym_env, lval_t* parent, err_t* err) {
  for (lval_t* x = parent; NIL != x; x = x->cdr) {
    x->car = eval_lval(sym_env, x->car, err);
  }

  return parent;
}

/*
*/
lval_t* eval_list(sym_env_t* sym_env, lval_t* args, err_t* err) {
  /* binding constructs and definitions and special procedures? */
  if (LVAL_SYM == args->car->type) {
    char* first_sym = args->car->identifier;
    if (!strcmp("quote", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return builtin_quote(sym_env, args, err);
    } else if (!strcmp("if", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return builtin_if(sym_env, args, err);
    } else if (!strcmp("lambda", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return builtin_lambda(sym_env, args, err);
    } else if (!strcmp("define", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return builtin_define(sym_env, args, err);
    } else if (!strcmp("set!", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return builtin_set(sym_env, args, err);
    } else if (!strcmp("begin", first_sym)) {
      decRef(l_pop(&args));  // remove id
      return proc_begin(sym_env, args, err);
    }
  }

  /* regular procedures */
  args = eval_children(sym_env, args, err);
  if (LVAL_ERR == args->type) {
    return args;
  }
  lval_t* func = l_pop(&args);

  /* make sure first arg is a func */
  if (LVAL_PROC != func->type) {
    decRef(func);
    decRef(args);
    return lval_err("1st element of list is not a func",
                    err); /* propagate error */
  }

  /* dispatch */
  if (func->builtin) {
    lval_t* x = func->builtin(sym_env, args, err);
    decRef(func);
    return x;
  }

  return dispatch_lambda(sym_env, func, args, err);
}

/* dipsatch eval function based on lval type
*/
lval_t* eval_lval(sym_env_t* sym_env, lval_t* v, err_t* err) {
  switch (v->type) {
    case LVAL_SYM:
      return eval_symbol(sym_env, v, err);
    case LVAL_PAIR:
      return eval_list(sym_env, v, err);
    case LVAL_PROC:
    case LVAL_NUM:
    case LVAL_BOOL:
    case LVAL_UNDEF:
    case LVAL_SPECIAL:
    case LVAL_ERR:
      return v;
    default:
      fucked_up("eval_lval", "I don't recognize this type yo");
      break;
  }
  return NULL; /* should not be reached, just to shut up compiler */
}

/*
*/
lval_t* dispatch_lambda(sym_env_t* sym_map,
                        lval_t* func,
                        lval_t* args,
                        err_t* err) {
  lval_t* params = lval_copy(func->parameters, err);
  lval_t* expression = lval_copy(func->exp, err);
  decRef(func);

  /* create symbol env */
  // TODO I am just overwriting sym_map here, why do I need to pass it at all?
  sym_map = populate(SYM_MAP, params, args, err);
  if (err->sig) {
    return lval_clean(expression, NULL);
  }  // couldn't populate

  /* evaluate */
  lval_t* return_val = eval_list(sym_map, expression, err);
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

/* Counts a list up to an upper bound
 * WARNING: assuming input is an regular list
*/
bool expect_count(lval_t* xs, uint16_t target) {
  uint16_t upper_bound = target + 1;
  uint16_t count = 0;
  lval_t* x = xs;
  while ((count != upper_bound) && (NIL != x)) {
    ++count;
    x = x->cdr;
  }

  return count == target;
}

//======================================
lval_t* builtin_define(sym_env_t* sym_env, lval_t* args, err_t* err) {
  /* Process arguments */
  if (!expect_count(args, 2)) {
    decRef(args);
    return lval_err("DEFINE : incorrect number of arguments", err);
  }

  lval_t* one = l_pop(&args);
  lval_t* x = lval_rip(args);

  if (!scheme_assert_type(LVAL_SYM, one)) {
    return lval_err("DEFINE : first arg should be a symbol", err);
  }
  char* symbol = rip_sym(one);

  /* eval */
  x = eval_lval(sym_env, x, err);
  if (err->sig) {
    free(symbol);
    return lval_clean(x, NULL);
  }

  /* push */
  /* note: adding directly to global SYM_MAP */
  if (symbol_add(SYM_MAP, symbol, x, err)) {
    return lval_err("DEFINE : symbol already bound", err);
  } else if (err->sig) { /* werent able to make space in the sym_map */
    printf("TODO global symbol stack error");
    exit(1);
  }

  return lval_undef(err);
}

/*
*/
lval_t* proc_begin(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);
  lval_t* r = NIL;
  while (x) {
    decRef(r);  // TODO is this right?
    r = eval_lval(sym_env, x, err);
    x = l_pop(&args);
  }

  decRef(args);
  return r;
}

//======================================
// FUNCTION: what you want to return is already in a list, just label it a
// qexp
lval_t* proc_list(sym_env_t* sym_env, lval_t* args, err_t* err) {
  if (NIL == args) {
    return NIL;
  }

  args->quoted = true;
  return args;
}

//======================================
lval_t* builtin_head(sym_env_t* sym_env, lval_t* args, err_t* err) {
  // check inputs
  /*
  if( 1 != args->count ) {
    return lval_err("head accepts exactly 1 arg", err);
  }
  */

  // TODO check arg is sexp?

  lval_t* list = lval_rip(args);
  /*
  if( list->count < 1 ) {
    return lval_err("head requires a list of length at least 1", err);
  }
  */

  // by this point: x is qexp of size > 1
  lval_t* head = lval_rip(list);

  // preserve quoting for nested expressions
  head->quoted = true;  // TODO is this right?

  return head;
}

//======================================
lval_t* builtin_tail(sym_env_t* sym_env, lval_t* x, err_t* err) {
  /*
  //check good state
  if( 1 != args->count ) {
    return lval_err("head accepts exactly 1 arg", err);
  }

  lval_t* arg1 = args->cell[0];
  if( !(arg1->count) ) {
    return lval_err("tail requires a list of length at least 1", err);
  }
  */

  x = lval_rip(x);   /* rip first arg and free the rest */
  decRef(l_pop(&x)); /* pop and discard the head */
  return x;          /* return the tail */
}

lval_t* builtin_join(sym_env_t* sym_env, lval_t* args, err_t* err) {
  return NULL;
}

//======================================
lval_t* builtin_eval(sym_env_t* sym_env, lval_t* args, err_t* err) {
  return NULL;
  //##  /*
  //##  if( 1 != args->count ) {
  //##    return lval_err("eval accepts exactly 1 arg", err);
  //##  }
  //##  */

  //##  lval_t* input = lval_rip(args); // the rest of ARGS will be freed here
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
lval_t* proc_sum(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);
  for (lval_t* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num += y->num;

    decRef(y);
  }

  decRef(args);
  return x;
}

/*
*/
lval_t* proc_minus(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);
  for (lval_t* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num -= y->num;

    decRef(y);
  }

  decRef(args);
  return x;
}

/*
*/
lval_t* proc_prod(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);
  for (lval_t* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num *= y->num;

    decRef(y);
  }

  decRef(args);
  return x;
}

/*
*/
lval_t* proc_quotient(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);
  for (lval_t* y = l_pop(&args); y; y = l_pop(&args)) {
    x->num /= y->num;

    decRef(y);
  }

  decRef(args);
  return x;
}

/*
*/
lval_t* builtin_lambda(sym_env_t* sym_env, lval_t* args, err_t* err) {
  lval_t* parameters = l_pop(&args);
  lval_t* exp = l_pop(&args);
  decRef(args);
  // TODO again, use lval_empty to check for preconditions
  // this will also involve making sure parameters and exp are not NULL
  return lval_lambda(parameters, exp, err);
}

/* look for symbol, if value found, return a copy
*/
lval_t* eval_symbol(sym_env_t* sym_env, lval_t* x, err_t* err) {
  char* sym = rip_sym(x);

  binding_t* binding = sym_search(sym_env, sym);
  if (!binding) {
    char* err_msg = malloc(
        (strlen("symbol [] is not bound") + 1 + strlen(sym)) * sizeof(char));
    sprintf(err_msg, "symbol [%s] is not bound", sym);
    free(sym);
    lval_t* a = lval_err(err_msg, err);
    free(err_msg);
    return a;
  }
  free(sym);

  /* return a copy of the value */
  /* TODO it should actually return a pointer to the value directly */
  return incRef(binding->value);
}

/********************************************************************************
* SYMBOL UTILS
********************************************************************************/
/*
*/
char* builtin_names[] = {"begin",    "eval",    "list",    "car",
                         "boolean?", "number?", "symbol?", "procedure?",
                         "pair?",    "eqv?",    "+",       "-",
                         "*",        "/",       "quote",   NULL};

/* dispatch builtin function based on string
*/
builtin_fun* proc_tab(char* x) {
  if (!strcmp("define", x)) {
    return builtin_define;
  } else if (!strcmp("quote", x)) {
    return builtin_quote;
  } else if (!strcmp("begin", x)) {
    return proc_begin;
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
    return proc_list;
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
sym_env_t* init_global_map(err_t* err) {
  sym_env_t* sym_map = sym_map_make(err);
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

  symbol_add(sym_map, strdup("#t", err), get_bool(true), err);
  symbol_add(sym_map, strdup("#f", err), get_bool(false), err);

  return sym_map;
}

/*
*/
void push_builtin(sym_env_t* sym_map, char* fun_name, err_t* err) {
  char* funx = strdup(fun_name, err);
  if (err->sig) {
    free(funx);
    return;
  }

  lval_t* xx = lval_builtin(proc_tab(fun_name), err);
  if (err->sig) {
    free(funx);
    decRef(xx);
    return;
  }

  symbol_add(sym_map, funx, xx, err);  // PPG ERR
}

/* note: this is the correct interface function to use (not `kv_push`)
** consumes: symbol, value
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

/* note: this will delete kv-chain but not parent
*/
void sym_map_del(sym_env_t* sym_map) {
  binding_t *next_binding, *binding = sym_map->first_child;
  while (binding) {
    next_binding = binding->sibling;
    free(binding->key);
    decRef(binding->value);  // debug
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

  /* debug */
  puts("parameters are: ");
  print_lval(parameters);
  puts("\n-----");
  /* debug */

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

/* consumes key
** consumes value
** modifies sym_map
** modifies error
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
    case LVAL_SPECIAL:
      return "NIL";
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
lval_t* builtin_quote(sym_env_t* sym_map, lval_t* args, err_t* err) {
  lval_t* x = l_pop(&args);  // WARNING not error checking
  return x;
}

/*
*/
bool scheme_true(lval_t* x) {
  if (LVAL_BOOL == x->type) {
    return x->is_true;
  }

  return true;
}

/*
*/
lval_t* builtin_if(sym_env_t* sym_map, lval_t* args, err_t* err) {
  lval_t* cond = l_pop(&args);
  lval_t* a = l_pop(&args);
  lval_t* b = l_pop(&args);
  decRef(args);

  cond = eval_lval(sym_map, cond, err);
  if (scheme_true(cond)) {
    lval_clean(cond, b);
    return eval_lval(sym_map, a, err);
  } else {
    lval_clean(cond, a);
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
bool scheme_assert_type(lval_type_t target, lval_t* x) {
  return target == x->type;
}

lval_t* scheme_type_predicate(lval_type_t target, lval_t* x, err_t* err) {
  bool a = scheme_assert_type(target, x);
  decRef(x);
  return get_bool(a);
}

lval_t* scm_rip_one_arg(lval_t* args) {
  lval_t* x = l_pop(&args);
  if (NIL != args) {
    puts("only one arg expected!");
    exit(1);
  }

  return x;
}

lval_t* proc_pred_bool(sym_env_t* sym_map, lval_t* args, err_t* err) {
  lval_t* x = scm_rip_one_arg(args);
  if (NIL == x) {
    return get_bool(false);
  }
  return scheme_type_predicate(LVAL_BOOL, x, err);
}

lval_t* proc_pred_num(sym_env_t* sym_map, lval_t* args, err_t* err) {
  lval_t* x = scm_rip_one_arg(args);
  if (NIL == x) {
    return get_bool(false);
  }
  return scheme_type_predicate(LVAL_NUM, x, err);
}

lval_t* proc_pred_sym(sym_env_t* sym_map, lval_t* args, err_t* err) {
  lval_t* x = scm_rip_one_arg(args);
  if (NIL == x) {
    return get_bool(false);
  }
  return scheme_type_predicate(LVAL_SYM, x, err);
}

lval_t* proc_pred_fun(sym_env_t* sym_map, lval_t* args, err_t* err) {
  lval_t* x = scm_rip_one_arg(args);
  if (NIL == x) {
    return get_bool(false);
  }
  return scheme_type_predicate(LVAL_PROC, x, err);
}

lval_t* proc_pred_pair(sym_env_t* sym_map, lval_t* args, err_t* err) {
  lval_t* x = scm_rip_one_arg(args);
  if (NIL == x) {
    return get_bool(false);
  }
  return scheme_type_predicate(LVAL_PAIR, x, err);
}

lval_t* proc_pred_eqv(sym_env_t* sym_map, lval_t* args, err_t* err) {
  lval_t* a = l_pop(&args);
  lval_t* b = l_pop(&args);
  decRef(args);

  /* deal with empty list */
  if ((NIL == a) && (NIL == b)) {
    return get_bool(true);
  }

  /* if not same type, can't be  eqv */
  if (a->type != b->type) {
    lval_clean(a, b);
    return get_bool(false);
  }

  bool x = false;
  switch (a->type) {
    case LVAL_BOOL:
      x = (a->is_true == b->is_true);
      break;
    case LVAL_SYM:
      x = !strcmp(a->identifier, b->identifier);
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

  lval_clean(a, b);
  return get_bool(x);
}

/*
*/
void scm_expect_type(lval_type_t type, lval_t* x) {
  if (NIL == x) {
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
lval_t* proc_car(sym_env_t* sym_env, lval_t* args, err_t* err) {
  if (!expect_count(args, 1)) {
    decRef(args);
    return lval_err("CAR : more than 1 arg", err);
  }

  lval_t* first = lval_rip(args);
  lval_t* x = first->car;
  incRef(x);
  decRef(first);  // TODO is this right?
  return x;
}

/*
*/
lval_t* builtin_set(sym_env_t* sym_env, lval_t* args, err_t* err) {
  /* set args */
  lval_t* variable = l_pop(&args);
  lval_t* expression = l_pop(&args);
  decRef(args);

  /* search for binding */
  char* sym = rip_sym(variable);
  binding_t* binding = sym_search(sym_env, sym);
  if (!binding) {
    char* err_msg = malloc((strlen("symbol [] is unbound") + 1 + strlen(sym)) *
                           sizeof(char));
    sprintf(err_msg, "symbol [%s] is unbound", sym);
    free(sym);
    lval_t* a = lval_err(err_msg, err);
    free(err_msg);
    return a;
  }
  free(sym);

  /* evaluate expression and set new value */
  lval_t* value = eval_lval(sym_env, expression, err);
  decRef(binding->value);
  binding->value = value;

  return lval_undef(err);
}

/*******************************************************************************
* FILE -> STRING
*******************************************************************************/
static unsigned get_file_size(const char* file_name) {
  struct stat sb;
  if (stat(file_name, &sb) != 0) {
    fprintf(stderr, "'stat' failed for '%s': %s.\n", file_name,
            strerror(errno));
    exit(EXIT_FAILURE);
  }
  return sb.st_size;
}

/* This routine reads the entire file into memory. */

static unsigned char* read_whole_file(const char* file_name) {
  unsigned s;
  unsigned char* contents = NULL;
  FILE* f;
  size_t bytes_read;
  int status;

  s = get_file_size(file_name);
  contents = malloc(s + 1);
  if (!contents) {
    fprintf(stderr, "Not enough memory.\n");
    exit(EXIT_FAILURE);
  }

  f = fopen(file_name, "r");
  if (!f) {
    fprintf(stderr, "Could not open '%s': %s.\n", file_name, strerror(errno));
    exit(EXIT_FAILURE);
  }
  bytes_read = fread(contents, sizeof(unsigned char), s, f);
  if (bytes_read != s) {
    fprintf(stderr,
            "Short read of '%s': expected %u bytes "
            "but got %lu: %s.\n",
            file_name, s, bytes_read, strerror(errno));
    exit(EXIT_FAILURE);
  }
  status = fclose(f);
  if (status != 0) {
    fprintf(stderr, "Error closing '%s': %s.\n", file_name, strerror(errno));
    exit(EXIT_FAILURE);
  }
  contents[s] = '\0';  // TODO double check this!
  return contents;
}
