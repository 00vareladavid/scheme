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
* only build one semantic peiece at a time, if a memory error occurs, you will know what to clean up
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
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "linenoise/linenoise.h"
#include "mpc/mpc.h"

/********************************************************************************
* UTILS
********************************************************************************/
int mallocfail = -5000;
#define oldmalloc(x) (malloc(x))
#define malloc(x) ( (1 == ++mallocfail) ? NULL : oldmalloc(x) )

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
void* x_malloc( size_t num, size_t size ){
  if( !size || !num ){
    return malloc(0);
  }

  size_t total = num * size;
  if( total < num ){
    fucked_up("malloc","integer oveflow");
  }

  return malloc(total);
}

size_t inc_size( size_t x ){
  x += 1u;

  if( !x ){
    fucked_up("inc_size", "unsigned carry(overflow)");
  }

  return x;
}

/********************************************************************************
* TYPES
********************************************************************************/
/* note a
*/
typedef enum builtin_t {NOT_A_BUILTIN,
	                BUILTIN_DEFINE, BUILTIN_LAMBDA, BUILTIN_EVAL,
	                BUILTIN_HEAD, BUILTIN_TAIL, BUILTIN_LIST, BUILTIN_JOIN,
			BUILTIN_PLUS, BUILTIN_MINUS, BUILTIN_MULT, BUILTIN_DIV,
                       }builtin_t;

/*
*/
typedef enum err_sig_t {OK, OUT_OF_MEM} err_sig_t;
typedef struct err_t {
  err_sig_t sig;
} err_t;

/*
*/
typedef enum lval_type_t { LVAL_NUM, LVAL_ERR, LVAL_SEXP,
	                   LVAL_SYM, LVAL_FUN, LVAL_UNDEF,
                         } lval_type_t;

struct lval; //TODO learn about forward declarations
typedef struct lval lval;
struct symbol_env;
typedef struct symbol_env symbol_env;

typedef lval* (builtin_fun)(symbol_env*, lval*, err_t* );

/*
*/
typedef struct lval {
  lval_type_t type;
  bool quoted;

  int64_t num; /* num */
  char* err; /* err */
  char* sym; /* symbol */

  /* functions */

  /* - builtin */
  builtin_fun *builtin;
  /* - lambda */
  lval* parameters; 
  lval* exp;

  /* sexp */
  lval* sibling;
  lval* first_child;
  lval* last_child;
} lval;

/*
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };
*/

/*
*/
typedef struct keyval {
  char* key;
  lval* value;
} keyval;

/*
*/
typedef struct symbol_map {
  keyval** mem;
  size_t count;
} symbol_map;

/*
*/
typedef struct symbol_env {
  symbol_map** stack;
  size_t count;
} symbol_env;

//======================================
symbol_env* make_symbol_env(err_t* err) {
  symbol_env* sym_env = malloc(sizeof(symbol_env));
  if( !sym_env ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  sym_env->stack = NULL;
  sym_env->count = 0;
  return sym_env;
}

/********************************************************************************
* PROTOTYPES
********************************************************************************/
//debugging
char* lval_type_string(lval* v );

//main
char* prompt(void);
void repl( mpc_parser_t* Lispy, symbol_env* sym_env, err_t* err );

//constructors
lval* lval_num(int64_t x, err_t* err);
lval* lval_err(char* err_msg, err_t* err);
lval* lval_sym(char* sym_string, err_t* err);
lval* lval_sexp(err_t* err);
lval* lval_qexp(err_t* err);
lval* lval_undef(err_t* err);
lval* lval_lambda(lval* parameters, lval* exp, err_t* err);

//lval utils
lval* lval_rip(lval* parent);
void lval_push(lval* v, lval* child);
lval* lval_pop(lval* parent);
void lval_join(lval* x, lval* y);

//lval del
void  lval_del(lval* v);
lval* lval_clean( lval *a, lval *b, lval *c );

//lval copy
lval* lval_copy(lval* v, err_t* err);
lval* lval_copy_sexp(lval* in, err_t* err);

//lval read
lval* lisp_read(mpc_ast_t* t, err_t* err);
lval* read_lval(mpc_ast_t* t, err_t* err);
lval* read_children(lval* parent, mpc_ast_t* t, err_t* err);
lval* read_num(mpc_ast_t* t, err_t* err);

//lval print
void print(lval* x);
void print_lval(lval* x);

//lval eval
lval* eval_lval(symbol_env*, lval* v, err_t* err);
lval* eval_sexp(symbol_env*, lval* v, err_t* err);
lval* builtin_op(symbol_env* sym_env, builtin_t builtin_code, lval* args, err_t* err);
lval* dispatch_builtin(symbol_env*, lval* func, lval* args, err_t* err);
lval* dispatch_lambda(symbol_env*, lval* func, lval* args, err_t* err);

/* BUILTINS */

/* most complex builtins, require all three args */
lval* builtin_define(symbol_env* sym_env, lval* args, err_t* err);
lval* builtin_eval(symbol_env* sym_env, lval* args, err_t* err);

/* these builtins operate directly on their args, no sym_env needed */
lval* builtin_lambda(symbol_env* sym_env, lval* args, err_t* err);
lval* builtin_head(symbol_env *sym_env, lval *args, err_t *err );
lval* builtin_tail(symbol_env *sym_env, lval *args, err_t *err );
lval* builtin_join(symbol_env *sym_env, lval *args, err_t *err );

/* safe builtins, no chance of memory errors */
lval* builtin_list(symbol_env *sym_env, lval *args, err_t *err );
lval* builtin_sum(symbol_env *sym_env, lval *args, err_t *err );
lval* builtin_minus(symbol_env *sym_env, lval *args, err_t *err );
lval* builtin_mult(symbol_env *sym_env, lval *args, err_t *err );
lval* builtin_div(symbol_env *sym_env, lval *args, err_t *err );

//env utils
symbol_env* init_symbol_env( err_t* );
builtin_fun* builtin_fun_map( char *x );
void push_builtin(symbol_map* sym_map, char* fun_name, err_t* err);
void free_symbol_map(symbol_map*);
void free_symbol_env(symbol_env* sym_env);

//symbol utils
bool symbol_add(symbol_map* sym_map, char* symbol, lval* value, err_t* err);
lval* eval_symbol( symbol_env* sym_env, lval* x, err_t* err );
lval* sym_map_search(symbol_map* sym_map, char* key);
lval* sym_search(symbol_env* env, char* key);
void push_kv(symbol_map* sym_map, char* key, lval* value, err_t* err);
void populate_symbol_env(symbol_env* sym_env, lval* parameters, lval* args, err_t* err);
symbol_map* make_symbol_map(err_t* err);
void symbol_env_push(symbol_env* sym_env, symbol_map* sym_map, err_t* err);
void symbol_env_pop(symbol_env*, err_t* err);

/********************************************************************************
* SHOULD BE IN string.h
********************************************************************************/
char* strdup(char* input, err_t* err) {
  char* x = calloc( inc_size(strlen(input)), sizeof(char) );
  if( !x ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  strcpy(x, input);
  return x;
}

/********************************************************************************
* MAIN
********************************************************************************/
int main(int argc, char* argv[] ) {
  /*** INIT ***/
  
  /* error struct */
  err_t* err = malloc(sizeof(err_t));
  if( !err ) {
    printf("1 insufficient mem for baseline framework\n");
    exit(1);
  }
  err->sig = OK;

  /* symbol env */
  symbol_env* sym_env = init_symbol_env(err);
  if( err->sig ){
    free(err);
    free_symbol_env(sym_env);
    printf("2 insufficient mem for baseline framework\n");
    exit(1);
  }

  /* parser */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexp = mpc_new("sexp");
  mpc_parser_t* Qexp = mpc_new("qexp");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT
    ," \
       number : /-?[0-9]+/ ;\
       symbol : '+' | '-' | '*' | '/' | /[a-zA-Z0-0_]+/ ;\
       sexp : '(' <expr>+ ')' ;\
       qexp : '{' <expr>+ '}' ;\
       expr : <number> | <symbol> | <sexp> | <qexp>;\
       lispy : /^/ <expr>+ /$/ ;\
     "
    ,Number ,Symbol ,Sexp ,Qexp ,Expr ,Lispy);
  
  /*** REPL ***/
  repl(Lispy, sym_env, err);
  
  /*** CLEAN ***/
  free(err);
  mpc_cleanup(6, Number, Symbol, Sexp, Qexp, Expr, Lispy);
  free_symbol_env(sym_env);
  return 0;
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
void repl( mpc_parser_t* Lispy, symbol_env* sym_env, err_t* err ){
  puts("Lispy v0.1.0");
  puts("Enter 'EXIT' to exit");

  mpc_result_t r; /* holds the ast */
  lval *x;
  char* input = prompt(); 
  while( strcmp(input,"QUIT") ){
    if( mpc_parse("<stdin>", input, Lispy, &r)) {
      /* read */
      x = lisp_read(r.output, err);
      if( err->sig ){
	printf("[ERR] Unable to READ lval due to insufficient memory\n");
	lval_del(x);

	err->sig = OK;
	goto clean_repl;
      }

      /* eval */
      x = eval_lval(sym_env, x, err);
      if( err->sig ){
	printf("[ERR] Unable to EVAL lval due to insufficient memory\n");
	lval_del(x);

	err->sig = OK;
	goto clean_repl;
      }

      /* print */
      print(x);
    } else {
      /* bad parse */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    clean_repl:
    free(input);
    input = prompt();
  }
  free(input);
}

/********************************************************************************
* CONSTRUCTORS
********************************************************************************/
lval* lval_num(int64_t number, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if( !v ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_NUM;
  v->num = number;
  return v;
}

/*
*/
lval* lval_err(char* err_msg, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if( !v ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_ERR;
  v->err = strdup(err_msg, err);
  if( err->sig ){ return lval_clean(v, NULL, NULL); }

  return v;
}

/* sym_string is only read
*/
lval* lval_sym(char* sym_string, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if( !v ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_SYM;
  v->sym = strdup(sym_string, err);
  if( err->sig ){ return lval_clean(v, NULL, NULL); }

  return v;
}

//======================================
lval* lval_sexp(err_t* err) {
  lval* v  = malloc(sizeof(lval));
  if( !v ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type  = LVAL_SEXP;
  v->quoted = false;
  v->first_child = NULL;
  v->last_child = NULL;
  return v;
}

//======================================
lval* lval_qexp(err_t* err) {
  lval* v = lval_sexp(err);
  if( err->sig ){
    return NULL;
  }

  v->quoted = true;
  return v;
}

//======================================
lval* lval_builtin(builtin_fun *fun, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if( !v ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_FUN;
  v->builtin = fun;
  return v;
}

//======================================
lval* lval_undef(err_t* err) {
  lval* x = malloc(sizeof(lval));
  if( !x ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  x->type = LVAL_UNDEF;
  return x;
}

//======================================
lval* lval_lambda(lval* parameters, lval* exp, err_t* err) {
  lval* x = malloc(sizeof(lval));
  if( !x ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  x->type = LVAL_FUN;
  x->builtin = NULL;
  x->parameters = parameters;
  x->exp = exp;
  return x;
}

/********************************************************************************
* LVAL UTILS
********************************************************************************/
/* Push
*/
void lval_push(lval* v, lval* child) {
  /* prep */
  child->sibling = NULL;

  if( v->last_child ){
    (v->last_child)->sibling = child;
    v->last_child = child;
  } else {
    v->first_child = child;
    v->last_child = child;
  }
}

/*
*/
lval* lval_pop(lval* parent) {
  lval* x = parent->first_child;
  if( x ){
    parent->first_child = x->sibling;
  }

  return x;
}

/*
*/
lval* lval_rip(lval* parent) {
  lval* x = lval_pop( parent );
  lval_del(parent);
  return x;
}

/*******************************************************************************
* DELETE
*******************************************************************************/

/* delete all children from a sexp/qexp
*/
void lval_del_children( lval* parent ){
  lval *sib;
  for( lval *a = parent->first_child; a; a = sib ){
    sib = a->sibling;
    lval_del(a);
  }
}

/* WARNING: this does not delete the struct itself, only the approriate elements
*/
void lval_del_func( lval *v ){
  /* if a lambda expression */
  if( !(v->builtin) ){
    lval_del(v->exp);
    lval_del(v->parameters);
  }

  /* nothing to delete for builtin functions */
}

/* recursively free the memory used by an lval
*/
void lval_del( lval *v ){
  if( !v ){ return; }

  switch( v->type ){
    case LVAL_NUM:
    case LVAL_UNDEF:
      break; /* do nothing */
    case LVAL_ERR:
      free(v->err);
      break;
    case LVAL_SYM:
      free(v->sym);
      break;
    case LVAL_SEXP:
      lval_del_children( v );
      break;
    case LVAL_FUN:
      lval_del_func(v);
      break;
    default:
      fucked_up("lval_del","I dont recognize this type yo");
      break;
  }

  free(v);
}

/* Free the memory used by up to three lvals
*/
lval* lval_clean( lval *a, lval *b, lval *c ){
  lval_del(a);
  lval_del(b);
  lval_del(c);
  return NULL;
}

/*******************************************************************************
* COPY
*******************************************************************************/

/* copy child list from src to des
*/
lval* lval_copy_sexp(lval* src, err_t* err){
  lval *dest = lval_sexp(err);
  if( err->sig ){ return NULL; }

  lval *x, *sib;
  for( x = src->first_child; x; x = sib ){
    /* record next sibling for later */
    sib = x->sibling;

    /* copy + push */
    x = lval_copy(x, err);
    if( err->sig ){ return lval_clean(dest, NULL, NULL); }
    lval_push(dest, x);
  }

  dest->quoted = src->quoted;
  return dest;
}

/* copy a lambda expression
*/
lval* copy_lambda(lval* src, err_t* err){
  lval *a = lval_copy(src->parameters, err);
  if( err->sig ){ return NULL; }
  lval *b = lval_copy(src->exp, err);
  if( err->sig ){ return lval_clean(a, NULL, NULL); }
  lval *x = lval_lambda(a,b,err);
  if( err->sig ){ return lval_clean(a, b, NULL); }
  return x;
}

/*
*/
lval* lval_copy_func( lval *v, err_t *err ){
  if( v->builtin ){
    return lval_builtin(v->builtin, err);
  }

  return copy_lambda(v, err);
}

/* purpose: dispatch copy function based on lval type
*/
lval* lval_copy( lval *v, err_t *err ){
  switch(v->type) {
    case LVAL_UNDEF:
      return lval_undef(err);
    case LVAL_NUM:
      return lval_num(v->num, err);
    case LVAL_FUN:
      return lval_copy_func(v, err);
    case LVAL_SYM:
      return lval_sym(v->sym, err);
    case LVAL_SEXP:
      return lval_copy_sexp(v, err);
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
typedef enum ast_tag_t { UNRECOGNIZED_TAG,
	                 NUMBER_TAG, SYMBOL_TAG, QEXP_TAG, SEXP_TAG, TOPLEVEL_TAG, } ast_tag_t;
ast_tag_t tag_map( char *tag ) {
  if( strstr(tag,"number") ) {
    return NUMBER_TAG;
  } else if( strstr(tag,"symbol") ) {
    return SYMBOL_TAG;
  } else if( strstr(tag,"qexp") ) {
    return QEXP_TAG;
  } else if( strstr(tag,"sexp") ) {
    return SEXP_TAG;
  } else if( !strcmp(tag,">") ) {
    return TOPLEVEL_TAG;
  } 

  return UNRECOGNIZED_TAG;
}

/* If read unsuccessful, will clean up ENTIRE parent tree
** ast is only read
*/
lval* read_children(lval* parent, mpc_ast_t* t, err_t* err) {
  uint32_t count = t->children_num;
  lval* next_child;
  for( size_t i = 0; i < count; i=inc_size(i) ){
    //if not a bracket, then it is a child value
    if( strcmp(t->children[i]->tag, "regex")
        && strncmp(t->children[i]->contents, "(", 1) 
        && strncmp(t->children[i]->contents, ")", 1)
        && strncmp(t->children[i]->contents, "{", 1) 
        && strncmp(t->children[i]->contents, "}", 1) ){
      next_child = read_lval(t->children[i], err);
      if( err->sig ){
        lval_del(parent);
	return NULL;
      }

      lval_push(parent, next_child);
    }
  }

  return parent;
}

/*
*/
lval* read_lval_sexp( mpc_ast_t* t, err_t* err ){
  lval *v = lval_sexp(err);
  if( err->sig ){
    lval_del(v);
    return NULL;
  }

  return read_children(v, t, err);
}

/*
*/
lval* read_lval_qexp( mpc_ast_t* t, err_t* err ){
  lval *v = lval_qexp(err);
  if( err->sig ){
    lval_del(v);
    return NULL;
  }

  v->quoted = true;

  return read_children(v, t, err);
}

/*
*/
lval* lisp_read( mpc_ast_t* t, err_t* err ){
  lval* x = read_lval(t, err); //propagate error
  mpc_ast_delete(t); //clean up
  return x; //propage error
}

/* purpose: dispatch reader function based on ast tag
*/
lval* read_lval( mpc_ast_t* t, err_t* err ){
  ast_tag_t tag = tag_map(t->tag);

  switch( tag ){
    case NUMBER_TAG:
      return read_num(t, err);
    case SYMBOL_TAG:
      return lval_sym(t->contents, err);
    case QEXP_TAG:
      return read_lval_qexp(t, err);
    case SEXP_TAG:
      return read_lval_sexp(t, err);
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
  int64_t num = strtol(t->contents,NULL,10);
  if( errno == ERANGE ) {
    return lval_err("number out of range", err);
  }

  return lval_num(num, err);
}

/********************************************************************************
* PRINT
********************************************************************************/
/*
*/
void print( lval* x ){
  print_lval(x);
  lval_del(x);
}

/*
*/
void print_lval_func( lval *x ){
  if( x->builtin ){
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
  lval *a;
  switch( x->type ) {
    case LVAL_NUM:
      printf("%ld ", x->num);
      break;
    case LVAL_SYM:
      printf("%s ", x->sym);
      break;
    case LVAL_ERR:
      printf("[ERROR: %s] ", x->err);
      break;
    case LVAL_SEXP:
      if( x->quoted ){
        printf("{");
      } else {
        printf("(");
      }
      a = x->first_child;
      while( a ){
        print_lval(a);
	a = a->sibling;
      }
      if( x->quoted ){
        printf("} ");
      } else {
        printf(") ");
      }
      break;
    case LVAL_UNDEF:
      printf("<undef> ");
      break;
    case LVAL_FUN:
      print_lval_func(x);
      break;
    default:
      fucked_up("print_lval","this type can't be printed, yo!");
      break;
  }
}

/********************************************************************************
* EVAL
********************************************************************************/
/* Remove and return the LL which represents a sexp from a LVAL_SEXP
** - should also work for QEXP
*/
lval* sexp_extract(lval* parent) {
  lval* x = parent->first_child;
  parent->first_child = NULL;
  parent->last_child = NULL;
  return x;
}

/*
*/
void eval_children(symbol_env* sym_env, lval* parent, err_t* err ){
  lval *chain = sexp_extract(parent);
  lval *x;
  while( chain ){
    x = chain;
    chain = x->sibling;
     
    x = eval_lval(sym_env, x, err);
    lval_push(parent, x);
  }
}

/* sym_env is modified
** v is consumed
** err is modified
*/
lval* eval_sexp( symbol_env* sym_env, lval* args, err_t* err ){
  eval_children(sym_env, args, err);
  lval* func = lval_pop(args);
  
  /* make sure first arg is a func */
  if( LVAL_FUN != func->type ){
    lval_del(func);
    lval_del(args);
    return lval_err("1st element of sexp list is not a func", err); /* propagate error */
  }

  /* dispatch */
  if( func->builtin ){
    lval *x = func->builtin(sym_env, args, err);
    lval_del(func);
    return x;
  }

  return dispatch_lambda(sym_env, func, args, err);
}

/* dipsatch eval function based on lval type
*/
lval* eval_lval( symbol_env* sym_env, lval* v, err_t* err ){
  switch( v->type ) {
    case LVAL_SYM:
      return eval_symbol(sym_env, v, err);
    case LVAL_SEXP:
      if( v->quoted ){
        return v;
      } else {
        return eval_sexp(sym_env, v, err);
      }
    case LVAL_NUM:
    case LVAL_UNDEF:
    case LVAL_FUN:
      return v;
    default:
      fucked_up("eval_lval", "I don't recognize this type yo");
      break;
  }
  return NULL; /* should not be reached, just to shut up compiler */
}

/*
*/
lval* dispatch_lambda(symbol_env* sym_env, lval* func, lval* args, err_t* err) {
  /* set up env */
  populate_symbol_env(sym_env, func->parameters, args, err); //parameters and args destroyed here
  //note, this is effective because the lval's being pushed are already constructed
  func->parameters = NULL;
  if( err->sig ){ return lval_clean(func,NULL,NULL); } //couldn't populate, pop back

  /* evaluate */
  func->exp->quoted = false;
  lval* return_val = eval_sexp(sym_env, func->exp, err);
  func->exp = NULL;
  if( err->sig ){ return lval_clean(func,NULL,NULL); }
  
  //clean up
  lval_del(func);
  symbol_env_pop(sym_env, err); //TODO fix error handling after refactoring to linked list
  // can't pop -> should be a fatal error since you won't leave th data ina consistent state
  // pop -> ok
  
  return return_val;
}

/*******************************************************************************
* SYMBOL UTILS
*******************************************************************************/
/*
*/
lval* sym_search(symbol_env* env, char* key) {
  lval* result;
  size_t index;
  symbol_map* next_map;
  for( size_t i = 0; i < env->count; i=inc_size(i) ){
    index = env->count - 1 - i;
    next_map = env->stack[index];
    result = sym_map_search(next_map, key);
    if( result ) {
      return result;
    }
  }

  return NULL;
}

//======================================
//TODO what if something becomes deallocated?
lval* sym_map_search(symbol_map* sym_map, char* key) {
  for( size_t i = 0; i < sym_map->count; i=inc_size(i) ){
    if( !strcmp(sym_map->mem[i]->key, key) ){
      return sym_map->mem[i]->value;
    }
  }

  return NULL;
}

//======================================
lval* builtin_define(symbol_env* sym_env, lval* args, err_t* err) {
  //TODO check preconditions
  
  /* set up pointers */
  /*
  if( 2 != args->count ) {
    return lval_err("define accepts exactly 2 args", err);
  }
  */

  lval* one = lval_pop(args);
  one = lval_rip(one); 
  lval* x = lval_pop(args);
  lval_del(args);

  /* rip symbol */
  if( LVAL_SYM != one->type ) {
    return lval_err("eval expects 1st arg to be of type LVAL_SYM", err);
  }
  char* symbol = one->sym;
  one->sym = NULL;
  lval_del(one);
  
  /* eval */
  x = eval_lval(sym_env, x, err);
  if( err->sig ){ return lval_clean(x, NULL, NULL); } /* couldn't eval def target */

  /* push */
  bool err_x = symbol_add(sym_env->stack[sym_env->count-1], symbol, x, err);//TODO remove this dirty hack
  if( err_x ){
    return lval_err("symbol already exists", err);
  }
  if( err->sig ){ /* werent able to make space in the sym_map on top of sym_env */
    //try to shrink sym_map back down
    // success -> then clean up and pop back
    // error -> fatal error
    printf("TODO global symbol stack error");
    exit(1);
  }

  return lval_undef(err);
}

//======================================
//FUNCTION: what you want to return is already in a list, just label it a qexp
lval* builtin_list( symbol_env *sym_env, lval *args, err_t *err ){
  args->type = LVAL_SEXP;
  args->quoted = true;
  return args;
}

//======================================
lval* builtin_head( symbol_env *sym_env, lval *args, err_t *err ){
  //check inputs
  /*
  if( 1 != args->count ) {
    return lval_err("head accepts exactly 1 arg", err);
  }
  */

  //TODO check arg is sexp?

  lval* list = lval_rip(args);
  /*
  if( list->count < 1 ) {
    return lval_err("head requires a list of length at least 1", err);
  }
  */

  //by this point: x is qexp of size > 1 
  lval* head = lval_rip(list);

  //preserve quoting for nested expressions
  if( !(head->quoted) ){
    head->quoted = true;
  }

  return head;
}

//======================================
lval* builtin_tail( symbol_env *sym_env, lval *x, err_t *err ){
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

  x = lval_rip(x); /* rip first arg and free the rest */
  lval_pop( x ); /* pop and discard the head */
  return x; /* return the tail */
}

//======================================
lval* builtin_join( symbol_env *sym_env, lval *args, err_t *err ){
  lval* master = lval_pop(args);
  //WARNING: what if you have a element which is just the NULL value?
  // The join could end prematurely and values would be lost
  //TODO will this be an issue?
  for( lval *next = lval_pop(args); next; next = lval_pop(args) ){
    lval_join(master, next);
  }

  lval_del(args);
  return master;
}

/* Transfer children from slave sexp to master sexp
** note: the child list will be transfered in order
*/
void lval_join(lval* master, lval* slave ){
  /* transfer chain */
  master->last_child->sibling = slave->first_child;
  master->last_child = slave->last_child;

  /* disown chain */
  slave->first_child = NULL;
  slave->last_child = NULL;
  
  /* clean up*/
  lval_del(slave);
}

//======================================
lval* builtin_eval( symbol_env* sym_env, lval* args, err_t* err ){
    /*
    if( 1 != args->count ) {
      return lval_err("eval accepts exactly 1 arg", err);
    }
    */

    lval* input = lval_rip(args);//the rest of ARGS will be freed here
    // TODO input = lval_pop(args); if( !sexp_empty(args) ){ lval_err(....
    if( !(input->quoted) ) {
      return lval_err("eval expected qexp", err);
    }

    input->type = LVAL_SEXP;
    return eval_lval(sym_env, input, err);
}

/* Note checking for overflow should be the job of LISP not here at a low level
*/
lval* builtin_sum( symbol_env *sym_env, lval *args, err_t *err ){
  lval *x = lval_pop(args);
  for( lval *y = lval_pop(args); y; y = lval_pop(args) ){
    x->num += y->num;

    lval_del(y);
  }

  lval_del(args);
  return x;
}

/*
*/
lval* builtin_minus( symbol_env *sym_env, lval *args, err_t *err ){
  lval* x = lval_pop( args );
  for( lval *y = lval_pop(args); y; y = lval_pop(args) ){
    x->num -= y->num;

    lval_del(y);
  }

  lval_del(args);
  return x;
}

/*
*/
lval* builtin_mult( symbol_env *sym_env, lval *args, err_t *err ){
  lval* x = lval_pop( args );
  for( lval *y = lval_pop(args); y; y = lval_pop(args) ){
    x->num *= y->num;

    lval_del(y);
  }

  lval_del(args);
  return x;
}

/*
*/
lval* builtin_div( symbol_env *sym_env, lval *args, err_t *err ){
  lval* x = lval_pop( args );
  for( lval *y = lval_pop(args); y; y = lval_pop(args) ){
    x->num /= y->num;

    lval_del(y);
  }

  lval_del(args);
  return x;
}

/*
*/
lval* builtin_lambda( symbol_env *sym_env, lval *args, err_t *err ){
  lval* parameters = lval_pop(args);
  lval* exp = lval_pop(args);
  //TODO again, use lval_empty to check for preconditions
  // this will also involve making sure parameters and exp are not NULL
  lval_del(args);
  return lval_lambda(parameters, exp, err);
}

/* look for symbol, if value found, return a copy
*/
lval* eval_symbol( symbol_env* sym_env, lval* x, err_t* err ){
  lval* v = sym_search(sym_env, x->sym);
  lval_del(x);

  if( !v ) {
    return lval_err("symbol not found", err);
  }
  
  return lval_copy(v, err);
}

/********************************************************************************
* SYMBOL UTILS
********************************************************************************/
/*
*/
char* builtin_names[] = {"define" ,"lambda" ,"eval"
	                ,"head" ,"tail" ,"list" ,"join"
                        ,"+" ,"-" ,"*" ,"/"
	                ,NULL };

/* dispatch builtin function based on string
*/
builtin_fun* builtin_fun_map( char *x ){
  if( !strcmp( "define", x ) ){
    return builtin_define;
  } else if( !strcmp( "lambda", x ) ){
    return builtin_lambda;
  } else if( !strcmp( "eval", x ) ){
    return builtin_eval;
  } else if( !strcmp( "head", x ) ){
    return builtin_head;
  } else if( !strcmp( "tail", x ) ){
    return builtin_tail;
  } else if( !strcmp( "list", x ) ){
    return builtin_list;
  } else if( !strcmp( "join", x ) ){
    return builtin_join;
  } else if( !strcmp( "+", x ) ){
    return builtin_sum;
  } else if( !strcmp( "-", x ) ){
    return builtin_minus;
  } else if( !strcmp( "*", x ) ){
    return builtin_mult;
  } else if( !strcmp( "/", x ) ){
    return builtin_div;
  } 

  fucked_up("builtin_fun_map", "unrecognized string");
  return 0;
}
  
/*
*/
symbol_env* init_symbol_env(err_t* err) {
  symbol_env* sym_env = make_symbol_env(err);
  if( err->sig ){
    return NULL;
  }

  //TODO refactor this into symbol_env_new_layer() and symbol_env_add_symbol()
  symbol_map* sym_map = make_symbol_map(err);
  if( err->sig ){
    free_symbol_map(sym_map);
    return sym_env;
  }

  char* x;
  for( size_t i = 0; (x = builtin_names[i]); i=inc_size(i) ){
    push_builtin(sym_map, x, err);
    if( err->sig ){
      free_symbol_map(sym_map);
      return sym_env;
    }
  }

  symbol_env_push(sym_env, sym_map, err);
  //just propogate error signal; nothing to clean
  return sym_env;
}

//======================================
void push_builtin(symbol_map* sym_map, char* fun_name, err_t* err) {
  char* funx = strdup(fun_name, err);
  if( err->sig ){
    free(funx);
    return;
  }

  lval* xx = lval_builtin(builtin_fun_map(fun_name), err);
  if( err->sig ){
    free(funx);
    lval_del(xx);
    return;
  }

  symbol_add(sym_map, funx, xx, err); // PPG ERR
}

//======================================
void free_symbol_map( symbol_map* sym_map ){
  if( !sym_map ){ return; }

  for( size_t i = 0; i < sym_map->count; i=inc_size(i) ){
    free(sym_map->mem[i]->key);
    lval_del(sym_map->mem[i]->value);
    free(sym_map->mem[i]);
  }

  free(sym_map->mem);
  free(sym_map);
  return;
}

//======================================
void free_symbol_env(symbol_env* sym_env) {
  if( !sym_env ){ return; }

  for( size_t i = 0; i < sym_env->count; i=inc_size(i) ){
    free_symbol_map(sym_env->stack[i]);
  }

  free(sym_env->stack);
  free(sym_env);
}

/* consumes: symbol, value
*/
bool symbol_add(symbol_map* sym_map, char* symbol, lval* value, err_t* err) {
  //make sure there is something in the symbol_map before you being searching
  if( sym_map && sym_map_search(sym_map, symbol) ) {
    return true;
  }

  keyval** new_mem = realloc(sym_map->mem, sizeof(keyval*) * (sym_map->count + 1));
  if( new_mem ){
    sym_map->mem = new_mem;
    sym_map->count++;

    sym_map->mem[sym_map->count - 1] = NULL;
  } else {
    err->sig = OUT_OF_MEM;
    free(symbol);
    lval_del(value);
    return false;
  }

  keyval* new_kv = malloc(sizeof(keyval));
  if( new_kv ){
    sym_map->mem[sym_map->count - 1] = new_kv;
  } else {
    err->sig = OUT_OF_MEM;
    free(symbol);
    lval_del(value);
    return false;  
  }

  sym_map->mem[sym_map->count - 1]->key = symbol;
  sym_map->mem[sym_map->count - 1]->value = value;
  return false;
}

//======================================
symbol_env* make_env(err_t* err) {
  symbol_env* env= malloc(sizeof(symbol_env));
  if( !env ){
   err->sig = OUT_OF_MEM;
   return NULL;
  }

  env->stack = NULL;
  env->count = 0;
  return env;
}

//======================================
symbol_map* make_symbol_map(err_t* err) {
  symbol_map* sym_map = malloc(sizeof(symbol_map));
  if( !sym_map ){
   err->sig = OUT_OF_MEM;
   return NULL;
  }

  sym_map->mem = NULL;
  sym_map->count = 0;
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
void populate_symbol_env( symbol_env* sym_env, lval* parameters, lval* args, err_t* err ){
  /* create blank slate; */
  symbol_map* sym_map = make_symbol_map(err);
  if( err->sig ){
    lval_clean( args, parameters, NULL );
    return;
  }

  /* populate */
  for( lval *a = lval_pop(parameters), *b = lval_pop(args);
       a;
       a = lval_pop(parameters), b = lval_pop(args) ){
    //make sure b is a valid value
    
    push_kv(sym_map, a->sym, b, err);
    if( err->sig ){ puts("pop sym env"); exit(1); }

    /* clean up */
    a->sym = NULL;
    lval_del( a );

    if( err->sig ){
      lval_clean( args, parameters, NULL );
      return;
    }
  }

  /* clean up */
  lval_clean( args, parameters, NULL );
  /* push */
  symbol_env_push(sym_env,sym_map, err);
}


/* consumes key
** consumes value
** modifies sym_map
** modifies error
*/
void push_kv(symbol_map* sym_map, char* key, lval* value, err_t* err) {
  /* make room */
  keyval** new_mem = realloc(sym_map->mem, sizeof(keyval*) * (sym_map->count + 1) );
  if( new_mem ){
    sym_map->mem = new_mem;
    sym_map->count++;
    new_mem[sym_map->count - 1] = NULL;
  } else {
    err->sig = OUT_OF_MEM;
    free(key);
    lval_del(value);
    return;
  }

  keyval* kv = malloc(sizeof(keyval));
  if( kv ){
    sym_map->mem[sym_map->count - 1] = kv;
  } else {
    err->sig = OUT_OF_MEM;
    free(key);
    lval_del(value);
    return;
  }

  /* set */
  kv->key = key;
  kv->value = value;
  return;
}

/*
*/
void symbol_env_pop(symbol_env* sym_env, err_t* err) {
  free_symbol_map(sym_env->stack[sym_env->count - 1]);
  sym_env->stack[sym_env->count - 1] = NULL;

  symbol_map** new_stack = realloc(sym_env->stack, sizeof(symbol_map*) * (sym_env->count - 1));
  if( new_stack ){
    sym_env->count--;
    sym_env->stack = new_stack;
  } else {
    err->sig = OUT_OF_MEM;
    return;
  }
}

/*
*/
void symbol_env_push(symbol_env* sym_env, symbol_map* sym_map, err_t* err) {
  /* make room */
  symbol_map** new_stack = realloc(sym_env->stack, sizeof(symbol_map*) * (sym_env->count + 1));
  if( new_stack ){
    sym_env->count++;
    sym_env->stack = new_stack;
    new_stack[sym_env->count - 1] = NULL;
  } else {
    err->sig = OUT_OF_MEM;
    free_symbol_map(sym_map);
    return;
  }

  /* push */
  sym_env->stack[sym_env->count - 1] = sym_map;
}

/******************************************************************************
* GRAVEYARD
******************************************************************************/
void print_chain( lval* chain ){
  puts("printing chain: ");
  while( chain ){
    print_lval(chain);
    chain = chain->sibling;
  }
  puts("chain**");
}

char* lval_type_string(lval* v ) {
  switch(v->type) {
    case LVAL_NUM:
      return "num";
    case LVAL_FUN:
      return "function";
    case LVAL_SYM:
      return "sym";
    case LVAL_ERR:
      return "err";
    case LVAL_SEXP:
      return "sexp";
    case LVAL_UNDEF:
      return "undef";
  }
  
  return "UNKNOWN TYPE";
}
