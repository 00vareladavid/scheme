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
--------------------------------------------------------------------------------
# After
* review C build process
* data structures and algorithms
--------------------------------------------------------------------------------
# Further reasearch
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise/linenoise.h"
#include "mpc/mpc.h"

/********************************************************************************
* UTILS
********************************************************************************/
int mallocfail = -5000;
#define oldmalloc(x) (malloc(x))
#define malloc(x) ( (1 == ++mallocfail) ? NULL : oldmalloc(x) )

void fucked_up(char* function_name, char* err_msg) {
  fprintf(stderr, "[ERROR] %s: %s\n", function_name, err_msg);
  exit(1);
}

/********************************************************************************
* TYPES
********************************************************************************/
typedef enum err_sig_t {OK, OUT_OF_MEM} err_sig_t;

typedef struct err_t {
  err_sig_t sig;
} err_t;

//======================================
typedef struct lval {
  unsigned type;
  long num;
  char* err;
  char* sym;

  //lambda
  struct lval* arg_list;
  struct lval* exp;

  unsigned count;
  struct lval** cell;
} lval;

enum { LVAL_NUM, LVAL_ERR, LVAL_SEXP, LVAL_QEXP, LVAL_SYM, LVAL_BUILTIN, LVAL_LAMBDA, LVAL_UNDEF};
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

//======================================
typedef struct keyval {
  char* key;
  lval* value;
} keyval;

//======================================
typedef struct symbol_map {
  keyval** mem;
  unsigned count;
} symbol_map;

//======================================
typedef struct symbol_env {
  symbol_map** stack;
  unsigned count;
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

//main
char* prompt(void);
void repl( mpc_parser_t* Lispy, symbol_env* sym_env, err_t* err );

//constructors
lval* lval_num(long x, err_t* err);
lval* lval_err(char* err_msg, err_t* err);
lval* lval_sym(char* sym_string, err_t* err);
lval* lval_sexp(err_t* err);
lval* lval_qexp(err_t* err);
lval* lval_undef(err_t* err);
lval* lval_lambda(lval* arg_list, lval* exp, err_t* err);

//lval utils
void  lval_del(lval* v);
lval* lval_copy(lval* v, err_t* err);
void lval_copy_cell(lval* out, lval* in, err_t* err);
unsigned lval_add(lval* v, lval* child, err_t* err);
lval* lval_pop(lval* sexp, unsigned index, err_t* err);
lval* lval_take(lval* sexp, unsigned index, err_t* err);
lval* lval_join(lval* x, lval* y, err_t* err);
char* lval_type_string(lval* v);

//reader utils
lval* lisp_read(mpc_ast_t* t, err_t* err);
lval* read_lval(mpc_ast_t* t, err_t* err);
void read_children(lval* parent, mpc_ast_t* t, err_t* err);
lval* read_num(mpc_ast_t* t, err_t* err);

//printer utils
void print(lval* x);
void print_lval(lval* x);

//eval utils
lval* eval_lval(symbol_env*, lval* v, err_t* err);
lval* eval_sexp(symbol_env*, lval* v, err_t* err);
lval* apply_op(char* op, lval* x, lval* y, err_t* err);
lval* builtin_op(symbol_env*, char* op, lval* v, err_t* err);
lval* dispatch_builtin(symbol_env*, lval* func, lval* args, err_t* err);
lval* dispatch_lambda(symbol_env*, lval* func, lval* args, err_t* err);

//env utils
symbol_env* init_symbol_env( err_t* );
void push_builtin(symbol_map* sym_map, char* fun_name, err_t* err);
unsigned free_symbol_map(symbol_map*);
unsigned free_symbol_env(symbol_env* sym_env);

//builtin functions
lval* builtin_define(symbol_env* sym_env, lval* args, err_t* err);
lval* builtin_eval(symbol_env* sym_env, lval* args, err_t* err);
lval* builtin_lambda(lval* args, err_t* err);
lval* builtin_head(lval* args, err_t* err);
lval* builtin_tail(lval* args, err_t* err);
lval* builtin_list(lval* args);
lval* builtin_append(lval* args, err_t* err);

//symbol utils
unsigned symbol_add(symbol_map* sym_map, char* symbol, lval* value, err_t* err);
lval* eval_symbol(symbol_env* sym_env, lval* x, err_t* err);
lval* sym_map_search(symbol_map* sym_map, char* key);
lval* sym_search(symbol_env* env, char* key);
unsigned push_kv(symbol_map* sym_map, char* key, lval* value, err_t* err);
unsigned populate_symbol_env(symbol_env* sym_env, lval* arg_list, lval* args, err_t* err);
unsigned sym_env_push(symbol_env* sym_env, symbol_map* sym_map, err_t* err);
symbol_map* make_symbol_map(err_t* err);
unsigned symbol_env_pop(symbol_env*, err_t* err);

/********************************************************************************
* SHOULD BE IN string.h
********************************************************************************/
char* strdup(char* input, err_t* err) {
  char* x = malloc(sizeof(char) * (strlen(input) + 1));
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
  puts("Enter 'QUIT' to quit");

  mpc_result_t r; /* holds the ast */
  lval *x;
  char* input = prompt(); 
  while( strcmp(input,"QUIT") ){
    if( mpc_parse("<stdin>", input, Lispy, &r)) {
      /* read */
      x = lisp_read(r.output, err);
      if( err->sig ){
	lval_del(x);
	printf("[ERR] Unable to READ lval due to insufficient memory\n");

	err->sig = OK;
	goto clean_repl;
      }

      /* eval */
      x = eval_lval(sym_env, x, err);
      if( err->sig ){
	lval_del(x);
	printf("[ERR] Unable to EVAL lval due to insufficient memory\n");

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

/*
*/
void print(lval* x) {
  print_lval(x);
  lval_del(x);
}

/********************************************************************************
* CONSTRUCTORS/DESTRUCTORS
********************************************************************************/
lval* lval_num(long number, err_t* err) {
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
  return v;
}

/*
*/
lval* lval_sym(char* sym_string, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if( !v ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_SYM;
  v->sym = strdup(sym_string, err);
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
  v->count = 0;
  v->cell  = NULL;
  return v;
}

//======================================
lval* lval_qexp(err_t* err) {
  lval* v = malloc(sizeof(lval));
  if( !v ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_QEXP;
  v->count = 0;
  v->cell = NULL;
  return v;
}

//======================================
lval* lval_builtin(char* func_key, err_t* err) {
  lval* v = malloc(sizeof(lval));
  if( !v ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  v->type = LVAL_BUILTIN;
  v->sym = strdup(func_key, err);
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
lval* lval_lambda(lval* arg_list, lval* exp, err_t* err) {
  lval* lambda = malloc(sizeof(lval));
  if( !lambda ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  lambda->type = LVAL_LAMBDA;
  lambda->arg_list = arg_list;
  lambda->exp = exp;
  return lambda;
}

//======================================
void lval_del(lval* v) {
  if( NULL == v) {
    //nothing to free
    return;
  }

  switch (v->type) {
    //do nothing for LVAL_NUM, LVAL_UNDEF
    case LVAL_NUM:
    case LVAL_UNDEF:
      break;
    case LVAL_ERR:
      free(v->err);
      break;
    case LVAL_BUILTIN:
    case LVAL_SYM:
      free(v->sym);
      break;
    case LVAL_QEXP:
    case LVAL_SEXP:
      for(unsigned i = 0; i < v->count; ++i) {
        lval_del(v->cell[i]);
      }
      free(v->cell);
      break;
    case LVAL_LAMBDA:
      lval_del(v->exp);
      lval_del(v->arg_list);
      break;
    default:
      fucked_up("lval_del","I dont recognize this type yo");
      break;
  }

  free(v);
}

//======================================
lval* lval_copy(lval* v, err_t* err) {
  lval* x = malloc(sizeof(lval));
  if( !x ){
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  x->type = v->type;
  switch(x->type) {
    //do nothing for LVAL_UNDEF
    case LVAL_UNDEF:
      break;
    case LVAL_NUM:
      x->num = v->num;
      break;
    case LVAL_BUILTIN:
    case LVAL_SYM:
      x->sym = strdup(v->sym, err);
      break;
    case LVAL_QEXP:
    case LVAL_SEXP:
      lval_copy_cell(x, v, err); 
      break;
    case LVAL_LAMBDA:
      x->arg_list = lval_copy(v->arg_list, err);
      x->exp = lval_copy(v->exp, err);
      break;
    default:
      fucked_up("lval_copy", "this type doesn't exist, yo!");
      break;
  }

  return x;
}

//======================================
void lval_copy_cell(lval* out, lval* in, err_t* err){
  lval** new_cell = malloc(sizeof(lval*) * in->count);
  if( new_cell ){
    out->count = in->count;
    out->cell = new_cell;
  } else {
    err->sig = OUT_OF_MEM;
    lval_del(out);
    return;
  }

  for(unsigned i = 0; i < out->count; ++i) {
    out->cell[i] = lval_copy(in->cell[i], err);
  }
}

/********************************************************************************
* READING
********************************************************************************/
lval* lisp_read( mpc_ast_t* t, err_t* err ){
  lval* x = read_lval(t, err);
  mpc_ast_delete(t);
  return x;
}

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

/* purpose: dispatch reader function based on ast tag
*/
lval* read_lval( mpc_ast_t* t, err_t* err ){
  lval* v = NULL;
  ast_tag_t tag = tag_map(t->tag);

  switch( tag ){
    case NUMBER_TAG:
      v = read_num(t, err);
      break;
    case SYMBOL_TAG:
      v = lval_sym(t->contents, err);
      break;
    case QEXP_TAG:
      v = lval_qexp(err);
      if( err->sig ){
        lval_del(v);
        return NULL;
      }

      read_children(v, t, err);
      break;
    case SEXP_TAG:
      v = lval_sexp(err);
      if( err->sig ){
        lval_del(v);
        return NULL;
      }

      read_children(v, t, err);
      break;
    case TOPLEVEL_TAG:
      v = read_lval(t->children[1], err);
      break;
    default:
      printf("ERROR unrecognized tag");
      exit(1);
  }
  return v;
}

//======================================
void read_children(lval* parent, mpc_ast_t* t, err_t* err) {
  unsigned count = t->children_num;
  lval* next_child;
  for(unsigned i = 0; i < count; ++i) {
    //if not a bracket, then it is a child value
    if( strcmp(t->children[i]->tag, "regex")
        && strncmp(t->children[i]->contents, "(", 1) 
        && strncmp(t->children[i]->contents, ")", 1)
        && strncmp(t->children[i]->contents, "{", 1) 
        && strncmp(t->children[i]->contents, "}", 1) ){
      next_child = read_lval(t->children[i], err);
      if( err->sig ){
        lval_del(next_child);
	return;
      }

      lval_add(parent, next_child, err);
      if( err->sig ){ return; }
    }
  }
}

//======================================
//append a child to sexp
unsigned lval_add(lval* v, lval* child, err_t* err) {
  lval** new_mem = realloc(v->cell, sizeof(lval*) * (v->count + 1) );
  if( NULL == new_mem ) {
    puts("***MEMORY ALLOCATION ERROR***");
    free(v->cell);
    exit(1);
  }
  v->count++;
  v->cell = new_mem;

  v->cell[v->count - 1] = child;
  return 0;
}

//======================================
lval* read_num(mpc_ast_t* t, err_t* err) {
  errno = 0;
  long num = strtol(t->contents,NULL,10);

  if( errno == ERANGE ) {
    return lval_err("number out of range", err);
  }

  return lval_num(num, err);
}

/********************************************************************************
* PRINT
********************************************************************************/
void print_lval(lval* x) {
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
      printf("(");
      for(unsigned i = 0; i < x->count; ++i ) {
        print_lval(x->cell[i]);
      }
      printf(")");
      break;
    case LVAL_QEXP:
      fputs("{",stdout);
      for(unsigned i = 0; i < x->count; ++i ) {
        print_lval(x->cell[i]);
      }
      fputs("} ",stdout);
      break;
    case LVAL_UNDEF:
      printf("<undef> ");
      break;
    case LVAL_LAMBDA:
      printf("<lambda ");
      print_lval(x->arg_list);
      print_lval(x->exp);
      printf("> ");
      break;
    default:
      fucked_up("print_lval","this type doesn't exist,yo!");
      break;
  }
}

/********************************************************************************
* EVAL
********************************************************************************/
//input will be destroyed
lval* eval_lval(symbol_env* sym_env, lval* v, err_t* err) {
  switch( v->type ) {
    case LVAL_SYM:
      return eval_symbol(sym_env, v, err);
      break;
    case LVAL_SEXP:
      return eval_sexp(sym_env, v, err);
      break;
    case LVAL_NUM:
    case LVAL_UNDEF:
    case LVAL_QEXP:
      return v;
      break;
    case LVAL_LAMBDA:
      return v;//TODO
      break;
    default:
      fucked_up("eval_lval", "I don't recognize this type yo");
      break;
  }

  return 0;
}

//======================================
lval* eval_sexp(symbol_env* sym_env, lval* v, err_t* err) {
  //assuming v is not empty;

  //split v into first_val and input args
  lval* first_val = lval_pop(v, 0, err);
  lval* args = v;//just renaming for clarity

  //call dispatch function based on function type
  lval* func = eval_lval(sym_env, first_val, err);
  switch( func->type ) {
    case LVAL_BUILTIN:
      return dispatch_builtin(sym_env, func, args, err);
      break;
    case LVAL_LAMBDA:
      return dispatch_lambda(sym_env, func, args, err);
      break;
    default:
      lval_del(func);
      lval_del(args);
      return lval_err("1st element of sexp list is not a func", err);
      break;
  }
}

//======================================
lval* dispatch_lambda(symbol_env* sym_env, lval* func, lval* args, err_t* err) {
  //preconditions
  if( args->count < func->arg_list->count ) {
    //clean up
    return lval_err("too few arguments", err);
  } else if ( args->count > func->arg_list->count ) {
    //clean up
    return lval_err("too many arguments", err);
  }

  //eval children
  for( unsigned i = 0; i < args->count; ++i ) {
    args->cell[i] = eval_lval(sym_env, args->cell[i], err);
  }

  //set up env
  populate_symbol_env(sym_env, func->arg_list, args, err); //arg_list and args destroyed here
  func->arg_list = NULL;

  //evaluate
  func->exp->type = LVAL_SEXP;
  lval* return_val = eval_sexp(sym_env, func->exp, err);
  func->exp = NULL;
  
  //clean up
  lval_del(func);
  symbol_env_pop(sym_env, err);
  return return_val;
}

//======================================
lval* sym_search(symbol_env* env, char* key) {
  lval* result;
  unsigned index;
  symbol_map* next_map;
  for(unsigned i = 0; i < env->count; ++i) {
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
  for(unsigned i = 0; i < sym_map->count; ++i) {
    if( !strcmp(sym_map->mem[i]->key, key) ){
      return sym_map->mem[i]->value;
    }
  }

  return NULL;
}

//======================================
lval* dispatch_builtin(symbol_env* sym_env, lval* func, lval* args, err_t* err) {
  lval* return_val;

  //SPECIAL functions
  if(!strcmp("define", func->sym)) {
    return_val = builtin_define(sym_env, args, err);
  } else {
    //IF NOT SPECIAL, EVAL ALL CHILDREN AND CALL FUNCTION

    //eval children
    for( unsigned i = 0; i < args->count; ++i ) {
      args->cell[i] = eval_lval(sym_env, args->cell[i], err);
    }

    //error handling
    for( unsigned i = 0; i < args->count; ++i ) {
      if(LVAL_ERR == args->cell[i]->type) {
        return lval_take(args, i, err);
      }
    }

    return_val = builtin_op(sym_env, func->sym, args, err);
  }

  //clean up and return
  lval_del(func);
  return return_val;
}

//======================================
lval* builtin_define(symbol_env* sym_env, lval* args, err_t* err) {
  if( 2 != args->count ) {
    return lval_err("eval accepts exactly 2 args", err);
  }

  if( LVAL_SYM != args->cell[0]->type ) {
    return lval_err("eval expects 1st arg to be of type LVAL_SYM", err);
  }

  //TODO error check 'symbol_add's return val

  //set up pointers and free unused memory
  char* symbol = args->cell[0]->sym;
  lval* x = args->cell[1];
  args->cell[0]->sym = NULL;
  args->cell[1] = NULL;
  lval_del(args);
  
  x = eval_lval(sym_env, x, err);
  unsigned err_x = symbol_add(sym_env->stack[sym_env->count-1], symbol,x, err);//TODO remove this dirty hack
  if( 1 == err_x ) {
    return lval_err("symbol already exists", err);
  }

  return lval_undef(err);
}

//======================================
//FUNCTION: what you want to return is already in a list, just label it a qexp
lval* builtin_list(lval* args) {
  args->type = LVAL_QEXP;
  return args;
}

//======================================
lval* builtin_head(lval* args, err_t* err) {
  //check inputs
  if( 1 != args->count ) {
    return lval_err("head accepts exactly 1 arg", err);
  }

  //TODO check arg is sexp?

  lval* list = lval_take(args,0, err);
  if( list->count < 1 ) {
    return lval_err("head requires a list of length at least 1", err);
  }

  //by this point: x is qexp of size > 1 
  lval* head = lval_take(list,0, err);

  //preserve quoting for nested expressions
  if( LVAL_SEXP == head->type ) {
    head->type = LVAL_QEXP;
  }

  return head;
}

//======================================
lval* builtin_tail(lval* args, err_t* err) {
  //check good state
  if( 1 != args->count ) {
    return lval_err("head accepts exactly 1 arg", err);
  }

  lval* arg1 = args->cell[0];
  if( !(arg1->count) ) {
    return lval_err("tail requires a list of length at least 1", err);
  }

  lval* list = lval_take(args,0, err);
  lval_del(lval_pop(list,0, err));
  return list;
}

//======================================
lval* builtin_append(lval* args, err_t* err) {
    //get size of next cell
    //realloc to fit both
    //memmove next cell

    lval* qexp = lval_pop(args,0, err);
    lval* next = NULL;
    while(args->count) {
      next = lval_pop(args,0, err);
      qexp = lval_join(qexp,next, err);
    }

    free(args);
    return qexp;
}

//======================================
//TODO add error checking
//TODO use memcpy for efficiency
lval* lval_join(lval* x, lval* y, err_t* err) {
  for(unsigned i = 0; i < y->count; ++i) {
    lval_add(x, y->cell[i], err);
  }
  lval_del(y);
  return x;
}

//======================================
lval* builtin_eval(symbol_env* sym_env, lval* args, err_t* err) {
    if( 1 != args->count ) {
      return lval_err("eval accepts exactly 1 arg", err);
    }

    lval* input = lval_take(args, 0, err);//the rest of ARGS will be freed here
    if( LVAL_QEXP != input->type ) {
      return lval_err("eval expected qexp", err);
    }

    input->type = LVAL_SEXP;
    return eval_lval(sym_env, input, err);
}

//======================================
// PURPOSE: dispatch function based on `op`
// NOTE: args will be destroyed
// TODO what if op is not a builtin?
lval* builtin_op(symbol_env* sym_env, char* op, lval* args, err_t* err) {

  if( !strcmp(op,"list") ) {
    return builtin_list(args);
  } else if( !strcmp(op,"head") ){
    return builtin_head(args, err);
  } else if( !strcmp(op, "tail") ){
    return builtin_tail(args, err);
  } else if( !strcmp(op, "append") ){
    return builtin_append(args, err);
  } else if( !strcmp(op, "eval") ){
    return builtin_eval(sym_env, args, err);
  } else if( !strcmp(op, "lambda") ){
    return builtin_lambda(args, err);
  }

  //arithmetic
  lval* x = lval_pop(args,0, err);
  while( args->count ) {
    lval* y = lval_pop(args,0, err);
    x = apply_op(op,x,y, err);
  }
  lval_del(args);
  return x;
}

//======================================
lval* builtin_lambda(lval* args, err_t* err) {
  //TODO preconditions
  lval* arg_list = lval_pop(args, 0, err);
  lval* exp = lval_pop(args, 0, err);
  lval_del(args); //TODO

  lval* lambda = lval_lambda(arg_list, exp, err);
  return lambda;
}

//======================================
//look for symbol, if value found, return a copy
lval* eval_symbol(symbol_env* sym_env, lval* x, err_t* err) {
  lval* v = sym_search(sym_env, x->sym);

  if( !v ) {
    lval_del(x);
    //TODO preallocate lval_errs
    return lval_err("symbol not found", err);
  }
  
  lval_del(x);
  return lval_copy(v, err);
}

//======================================
lval* apply_op(char* op, lval* x, lval* y, err_t* err) {
  if(!strcmp(op,"+")) {
    x->num = x->num + y->num;
  } else if (!strcmp(op,"-")) {
    x->num = x->num - y->num;
  } else if (!strcmp(op,"*")) {
    x->num = x->num * y->num;
  } else if (!strcmp(op,"/")) {
    x->num = x->num / y->num;
  } else {
    //TODO this should be an exit condition
    return lval_err("invalid symbol", err);
  }

  lval_del(y);
  return x;
}

/********************************************************************************
* LVAL UTILS
********************************************************************************/
lval* lval_pop(lval* sexp, unsigned index, err_t* err) {
  //#special case of a single element
  if( 1 == sexp->count ){
    lval* r = sexp->cell[index];
    free(sexp->cell);
    sexp->cell = NULL;
    sexp->count = 0;
    return r;
  }

  //#general case
  lval* x = lval_copy(sexp->cell[index], err);//this is a dirty hack for now
  lval_del(sexp->cell[index]);
  sexp->cell[index] = NULL;
  if( err->sig ){
    printf("too lazy to implement this\n");
    exit(1);
  }
  //this workaround will become a non issue when linked lists are used instead of arrays
  
  //shift
  memmove(&sexp->cell[index],
          &sexp->cell[index+1],
          sizeof(lval*) * (sexp->count - (index + 1)));

  //reflect true size
  lval** new_cell = realloc(sexp->cell, sizeof(lval*) * (sexp->count - 1));
  if( new_cell ){
    sexp->count--;
    sexp->cell = new_cell;
  } else {
    err->sig = OUT_OF_MEM;
    return 0;
  }

  return x;
}

//======================================
lval* lval_take(lval* sexp, unsigned index, err_t* err) {
  lval* x = lval_pop(sexp, index, err);
  lval_del(sexp);

  return x;
}

//======================================
char* lval_type_string(lval* v ) {
  switch(v->type) {
    case LVAL_NUM:
      return "num";
      break;
    case LVAL_BUILTIN:
      return "builtin";
      break;
    case LVAL_SYM:
      return "sym";
      break;
    case LVAL_ERR:
      return "err";
      break;
    case LVAL_SEXP:
      return "sexp";
      break;
    case LVAL_QEXP:
      return "qexp";
      break;
    case LVAL_UNDEF:
      return "undef";
      break;
    case LVAL_LAMBDA:
      return "lambda";
      break;
    default:
      fucked_up("lval_type_string", "i dont recognize this type");
      break;
  }
  
  return "UNKNOWN TYPE";
}

/********************************************************************************
* SYMBOL UTILS
********************************************************************************/
//======================================
//add all builtin keywords

char* builtin_names[] = {"define" ,"lambda" 
	                ,"head" ,"tail" ,"list" ,"join"
                        ,"+" ,"-" ,"*" ,"/"
	                ,NULL };
  
//======================================
symbol_env* init_symbol_env(err_t* err) {
  symbol_env* sym_env = make_symbol_env(err);
  if( err->sig ){
    return sym_env;
  }

  //TODO refactor this into symbol_env_new_layer() and symbol_env_add_symbol()
  symbol_map* sym_map = make_symbol_map(err);
  if( err->sig ){
    free_symbol_map(sym_map);
    return sym_env;
  }

  char* x;
  for(unsigned i = 0; (x = builtin_names[i]); ++i) {
    push_builtin(sym_map, x, err);
    if( err->sig ){
      free_symbol_map(sym_map);
      return sym_env;
    }
  }

  sym_env_push(sym_env, sym_map, err);
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

  lval* xx = lval_builtin(fun_name, err);
  if( err->sig ){
    free(funx);
    lval_del(xx);
    return;
  }

  symbol_add(sym_map, funx, xx, err);
  //just propogate the error signal upwards; nothing to clean up
}

//======================================
unsigned free_symbol_map(symbol_map* sym_map) {
  if( !sym_map ){ return 0; }

  for(unsigned i = 0; i < sym_map->count; ++i) {
    free(sym_map->mem[i]->key);
    lval_del(sym_map->mem[i]->value);
    free(sym_map->mem[i]);
  }

  free(sym_map->mem);
  free(sym_map);
  return 0;
}

//======================================
unsigned free_symbol_env(symbol_env* sym_env) {
  if( !sym_env ){ return 0; }

  for(unsigned i = 0; i < sym_env->count; ++i) {
    free_symbol_map(sym_env->stack[i]);
  }

  free(sym_env->stack);
  free(sym_env);
  return 0;
}

//======================================
//WARNING: this expects symbol and value to already be allocated!!!
unsigned symbol_add(symbol_map* sym_map, char* symbol, lval* value, err_t* err) {
  //make sure there is something in the symbol_map before you being searching
  if( sym_map && sym_map_search(sym_map, symbol) ) {
    return 1;
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
    return 0;
  }

  keyval* new_kv = malloc(sizeof(keyval));
  if( new_kv ){
    sym_map->mem[sym_map->count - 1] = new_kv;
  } else {
    err->sig = OUT_OF_MEM;
    free(symbol);
    lval_del(value);
    return 0;  
  }

  sym_map->mem[sym_map->count - 1]->key = symbol;
  sym_map->mem[sym_map->count - 1]->value = value;
  
  return 0;
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

//======================================
// WARNING: this function assumes:
// * arg_list->count == args->count
// * arg_list is a sexp with only LVAL_SYM as children
// * args is a sexp
unsigned populate_symbol_env(symbol_env* sym_env, lval* arg_list, lval* args, err_t* err) {
  //create blank slate;
  symbol_map* sym_map = make_symbol_map(err);

  //populate
  unsigned count = arg_list->count;
  for(unsigned i = 0; i < count; ++i) {
    push_kv(sym_map, arg_list->cell[i]->sym, args->cell[i], err);
    //cut used pointers
    arg_list->cell[i]->sym = NULL;
    args->cell[i] = NULL;
  }
  //delete from root
  lval_del(arg_list);
  lval_del(args);
  
  //push
  sym_env_push(sym_env,sym_map, err);

  return 0;
}

//======================================
//TODO inputs are being destroyed, is this correct?
unsigned push_kv(symbol_map* sym_map, char* key, lval* value, err_t* err) {
  //make room
  keyval** new_mem = realloc(sym_map->mem, sizeof(keyval*) * (sym_map->count + 1) );
  if( new_mem ){
    sym_map->mem = new_mem;
    sym_map->count++;
    new_mem[sym_map->count - 1] = NULL;
  } else {
    err->sig = OUT_OF_MEM;
    return 0;
  }

  keyval* kv = malloc(sizeof(keyval));
  if( kv ){
    sym_map->mem[sym_map->count - 1] = kv;
  } else {
    err->sig = OUT_OF_MEM;
    return 0;
  }

  kv->key = key;
  kv->value = value;
  return 0;
}

//======================================
unsigned symbol_env_pop(symbol_env* sym_env, err_t* err) {
  free_symbol_map(sym_env->stack[sym_env->count - 1]);

  symbol_map** new_stack = realloc(sym_env->stack, sizeof(symbol_map*) * (sym_env->count - 1));
  if( new_stack ){
    sym_env->count--;
    sym_env->stack = new_stack;
  } else {
    err->sig = OUT_OF_MEM;
    return 0;
  }

  return 0;
}

//======================================
unsigned sym_env_push(symbol_env* sym_env, symbol_map* sym_map, err_t* err) {
  //make room
  symbol_map** new_stack = realloc(sym_env->stack, sizeof(symbol_map*) * (sym_env->count + 1));
  if( new_stack ){
    sym_env->count++;
    sym_env->stack = new_stack;
    new_stack[sym_env->count - 1] = NULL;
  } else {
    err->sig = OUT_OF_MEM;
    free_symbol_map(sym_map);
    return 0;
  }

  //push
  sym_env->stack[sym_env->count - 1] = sym_map;
  return 0;
}
