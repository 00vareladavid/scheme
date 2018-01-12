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
--------------------------------------------------------------------------------
# After
* review C build process
* data structures and algorithms
--------------------------------------------------------------------------------
# Further reasearch
* how does lval_pop work?
 * **why doesn't x get overriden on memmove?**
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise/linenoise.h"
#include "mpc/mpc.h"

/*
********************************************************************************
UTILS
********************************************************************************
*/
void fucked_up(char* function_name, char* err_msg) {
  fprintf(stderr, "[ERROR] %s: %s\n", function_name, err_msg);
  exit(1);
}

/*
********************************************************************************
TYPES
********************************************************************************
*/
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

typedef struct symbol_map {
  keyval** mem;
  unsigned count;
} symbol_map;

typedef struct symbol_env {
  symbol_map** stack;
  unsigned count;
} symbol_env;

symbol_env* make_symbol_env(void) {
  symbol_env* sym_env = malloc(sizeof(symbol_env));
  sym_env->stack = NULL;
  sym_env->count = 0;
  return sym_env;
}

unsigned sym_env_push(symbol_env* sym_env, symbol_map* sym_map) {
  //make room
  sym_env->count++;
  sym_env->stack = realloc(sym_env->stack, sym_env->count);
  //TODO error check
  //push
  sym_env->stack[sym_env->count-1] = sym_map;

  return 0;
}

/*
********************************************************************************
PROTOTYPES
********************************************************************************
*/

//constructors
lval* lval_num(long x);
lval* lval_err(char* err_msg);
lval* lval_sym(char* sym_string);
lval* lval_sexp(void);
lval* lval_qexp(void);
lval* lval_undef(void);
lval* lval_lambda(lval* arg_list, lval* exp);

//lval utils
void  lval_del(lval* v);
unsigned lval_copy(lval* src, lval** dest);
unsigned lval_add(lval* v, lval* child);
lval* lval_pop(lval* sexp, unsigned index);
lval* lval_take(lval* sexp, unsigned index);
lval* lval_join(lval* x, lval* y);
char* lval_type_string(lval* v );

//reader utils
lval* read_lval(mpc_ast_t* t);
lval* read_num(mpc_ast_t* t);

//printer utils
void print_lval(lval* x);

//eval utils
lval* eval_lval(symbol_env*, lval* v);
lval* eval_sexp(symbol_env*, lval* v);
lval* apply_op(char* op, lval* x, lval* y);
lval* builtin_op(symbol_env*, char* op, lval* v);
lval* dispatch_builtin(symbol_env*, lval* func, lval* args);
lval* dispatch_func(symbol_env*, lval* func, lval* args);

//env utils
unsigned init_symbol_env(symbol_env*);
unsigned free_symbol_map(symbol_map*);
unsigned free_symbol_env(symbol_env* sym_env);

//builtin functions
lval* builtin_define(symbol_env* sym_env, lval* args);
lval* builtin_eval(symbol_env* sym_env, lval* args);
lval* builtin_lambda(lval* args);
lval* builtin_head(lval* args);
lval* builtin_tail(lval* args);
lval* builtin_list(lval* args);
lval* builtin_append(lval* args);

//symbol utils
unsigned symbol_add(symbol_map* sym_map, char* symbol, lval* value);
lval* symbol_find(symbol_map* sym_map, char* x);
lval* eval_symbol(symbol_env* sym_env, lval* x);
lval* sym_map_search(symbol_map* sym_map, char* key);
lval* sym_search(symbol_env* env, char* key);
unsigned push_kv(symbol_map* sym_map, char* key, lval* value);
unsigned populate_symbol_env(symbol_env* sym_env, lval* arg_list, lval* args);
unsigned sym_env_push(symbol_env* sym_env, symbol_map* sym_map);
symbol_map* make_symbol_map(void);
unsigned symbol_env_pop(symbol_env*);

/*
********************************************************************************
SHOULD BE IN string.h
********************************************************************************
*/
char* strdup(char* input) {
  char* x = malloc(sizeof(char) * (strlen(input) + 1));
  strcpy(x,input);
  return x;
}

/*
********************************************************************************
MAIN
********************************************************************************
*/
int main(int argc, char* argv[] ) {
//INIT
  symbol_env* sym_env= make_symbol_env();
  init_symbol_env(sym_env);

  //Parser
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
  
//REPL
  puts("Lispy v0.1.0");
  puts("Enter 'quit' to quit\n");

  while(1) {
    //prompt
    char* input = linenoise("> ");
    linenoiseHistoryAdd(input);

    //TODO add appropriate flags to linenoise to handle escape sequences
    //check for escape codes
    if(strcmp(input,"quit") == 0) {
      free(input);
      break;
    }

    //parsing
    mpc_result_t r;
    if( mpc_parse("<stdin>", input, Lispy, &r)) {
      //parsed good, then eval
      //mpc_ast_print(r.output);//DEBUG

      lval* x = read_lval(r.output);
      lval* y = eval_lval(sym_env, x);
      print_lval(y);
      puts("\n========================================");

      //clean (in reverse order);
      lval_del(y);
      mpc_ast_delete(r.output);
    } else {
      //parsed bad
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    //clean up
    free(input);
  }

  //clean up
  mpc_cleanup(4, Number, Symbol, Sexp, Qexp, Expr, Lispy);
  free_symbol_env(sym_env);
  return 0;
}

/*
********************************************************************************
CONSTRUCTORS/DESTRUCTORS
********************************************************************************
*/
lval* lval_num(long number) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = number;

  return v;
}

//======================================
lval* lval_err(char* err_msg) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(sizeof(char) * (strlen(err_msg) + 1));
  strcpy(v->err, err_msg);

  return v;
}

//======================================
lval* lval_sym(char* sym_string) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym  = malloc(sizeof(char) * (strlen(sym_string) + 1));
  strcpy(v->sym, sym_string);
  return v;
}

//======================================
lval* lval_sexp(void) {
  lval* v  = malloc(sizeof(lval));
  v->type  = LVAL_SEXP;
  v->count = 0;
  v->cell  = NULL;
  return v;
}

//======================================
lval* lval_qexp(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXP;
  v->count = 0;
  v->cell = NULL;
  return v;
}

//======================================
lval* lval_builtin(char* func_key) {
  lval* x = lval_sym(func_key);
  x->type = LVAL_BUILTIN;
  return x;
}

//======================================
lval* lval_undef(void) {
  lval* x = malloc(sizeof(lval));
  x->type = LVAL_UNDEF;
  return x;
}

//======================================
lval* lval_lambda(lval* arg_list, lval* exp) {
  lval* lambda = malloc(sizeof(lval));
  lambda->type = LVAL_LAMBDA;
  lambda->arg_list = arg_list;
  lambda->exp = exp;
  return lambda;
}

//======================================
void lval_del(lval* v) {
  if( NULL == v) {
    return;
  }

  switch (v->type) {
    //do nothing for LVAL_NUM, LVAL_UNDEF
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
    default:
      fucked_up("lval_del","I dont recognize this type yo");
      break;
  }

  free(v);
}

//======================================
unsigned lval_copy(lval* src, lval** dest) {
  *dest = malloc(sizeof(lval));
  lval* y = *dest;
  y->type = src->type;
  switch(y->type) {
    //do nothing for LVAL_UNDEF
    case LVAL_NUM:
      y->num = src->num;
      break;
    case LVAL_BUILTIN:
    case LVAL_SYM:
      y->sym = malloc(sizeof(char) * (strlen(src->sym) + 1));
      strcpy(y->sym, src->sym);
      break;
    case LVAL_QEXP:
    case LVAL_SEXP:
      y->count = src->count;
      y->cell = malloc(sizeof(lval*) * y->count);
      for(unsigned i = 0; i < y->count; ++i) {
        lval_copy(src->cell[i], &(y->cell[i]));
        //y->cell[i] = lval_copy(src->cell[i]);
      }
      break;
    case LVAL_LAMBDA:
      lval_copy(src->arg_list, &(y->arg_list));
      //y->arg_list = lval_copy(src->arg_list);
      lval_copy(src->exp, &(y->exp));
      //y->exp = lval_copy(src->exp);
      break;
    default:
      fucked_up("lval_copy", "this type doesn't exist, yo!");
      break;
  }

  return 0;
}

/*
********************************************************************************
READING
********************************************************************************
*/
lval* read_lval(mpc_ast_t* t) {
  if( strstr(t->tag,"number") ) {
    //puts("num");
    return read_num(t);
  } else if( strstr(t->tag,"symbol") ) {
    //puts("sym");
    return lval_sym(t->contents);
  } else if( strstr(t->tag,"qexp") ) {
    //puts("qexp");
    lval* v = lval_qexp();
    unsigned count = t->children_num;

    //if not a bracket, then it is a child value
    for(unsigned i = 0; i < count; ++i ) {
      if( strncmp(t->children[i]->contents, "{", 1) 
          && strncmp(t->children[i]->contents, "}", 1)
          && strcmp(t->children[i]->tag, "regex") ){
        lval_add(v, read_lval(t->children[i]));
      } 
    }

    return v;
  } else if( strstr(t->tag,"sexp") ) {
    //puts("**sexp");
    lval* v = lval_sexp();
    unsigned count = t->children_num;

    //if not a bracket, then it is a child value
    for(unsigned i = 0; i < count; ++i) {
      if( strncmp(t->children[i]->contents, "(", 1) 
          && strncmp(t->children[i]->contents, ")", 1)
          && strcmp(t->children[i]->tag, "regex") ){
        lval_add(v, read_lval(t->children[i]));
      }
    }

    return v;
  } else if( !strcmp(t->tag,">") ) {
    //toplevel
    return read_lval(t->children[1]);
  } else {
    return lval_undef();
  }
}

//======================================
//append a child to sexp
unsigned lval_add(lval* v, lval* child) {
  v->count++;

  lval** new_mem = realloc(v->cell, sizeof(lval*) * v->count );
  if( NULL == new_mem ) {
    puts("***MEMORY ALLOCATION ERROR***");
    free(v->cell);
    exit(1);
  }
  v->cell = new_mem;

  v->cell[v->count - 1] = child;
  return 0;
}

//======================================
lval* read_num(mpc_ast_t* t) {
  errno = 0;
  long num = strtol(t->contents,NULL,10);

  if( errno == ERANGE ) {
    return lval_err("number out of range");
  }

  return lval_num(num);
}

/*
********************************************************************************
PRINT
********************************************************************************
*/
void print_lval(lval* x) {
  switch( x->type ) {
    case LVAL_NUM:
      //puts("num");
      printf("%ld ", x->num);
      break;
    case LVAL_SYM:
      //puts("sym");
      printf("%s ", x->sym);
      break;
    case LVAL_ERR:
      //puts("err");
      printf("[ERROR: %s] ", x->err);
      break;
    case LVAL_SEXP:
      //puts("sexp");
      printf("(");
      for(unsigned i = 0; i < x->count; ++i ) {
        print_lval(x->cell[i]);
      }
      printf(")");
      break;
    case LVAL_QEXP:
      //puts("qexp");
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

/*
********************************************************************************
EVAL
********************************************************************************
*/
//input will be destroyed
lval* eval_lval(symbol_env* sym_env, lval* v) {
  switch( v->type ) {
    case LVAL_SYM:
      return eval_symbol(sym_env, v);
      break;
    case LVAL_SEXP:
      return eval_sexp(sym_env, v);
      break;
    case LVAL_NUM:
    case LVAL_UNDEF:
    case LVAL_QEXP:
      return v;
      break;
    case LVAL_LAMBDA:
      return v;//TODO
      break;
  }

  return 0;
}

//======================================
// input: `v` will be freed
lval* eval_sexp(symbol_env* sym_env, lval* v) {
  //assuming v is not empty;

  //split v into first_val and input args
  lval* first_val = lval_pop(v, 0);
  lval* args = v;//just renaming for clarity

  //call dispatch function based on function type
  lval* func = eval_lval(sym_env, first_val);
  switch( func->type ) {
    case LVAL_BUILTIN:
      return dispatch_builtin(sym_env, func, args);
      break;
    case LVAL_LAMBDA:
      return dispatch_func(sym_env, func, args);
      break;
    default:
      lval_del(func);
      lval_del(args);
      return lval_err("1st element of sexp list is not a func");
      break;
  }
}

//======================================
lval* dispatch_func(symbol_env* sym_env, lval* func, lval* args) {
  //preconditions
  if( args->count < func->arg_list->count ) {
    //clean up
    return lval_err("too few arguments");
  } else if ( args->count > func->arg_list->count ) {
    //clean up
    return lval_err("too many arguments");
  }

  //eval children
  for( unsigned i = 0; i < args->count; ++i ) {
    args->cell[i] = eval_lval(sym_env, args->cell[i]);
  }

  //set up env
  populate_symbol_env(sym_env, func->arg_list, args);

  //evaluate
  lval* exp = func->exp;
  exp->type = LVAL_SEXP;
  lval* return_val = eval_sexp(sym_env, exp);
  
  //clean up
  symbol_env_pop(sym_env);
  lval_del(func);
  return return_val;
}

//======================================
lval* sym_search(symbol_env* env, char* key) {
  unsigned index;
  for(unsigned i = 0; i < env->count; ++i) {
    index = env->count - 1 - i;
    symbol_map* next_map = env->stack[index];
    lval* result = sym_map_search(next_map, key);
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
lval* dispatch_builtin(symbol_env* sym_env, lval* func, lval* args) {
  lval* return_val;

  //SPECIAL functions
  if(!strcmp("define", func->sym)) {
    return_val = builtin_define(sym_env, args);
  } else {
    //IF NOT SPECIAL, EVAL ALL CHILDREN AND CALL FUNCTION

    //eval children
    for( unsigned i = 0; i < args->count; ++i ) {
      args->cell[i] = eval_lval(sym_env, args->cell[i]);
    }

    //error handling
    for( unsigned i = 0; i < args->count; ++i ) {
      if(LVAL_ERR == args->cell[i]->type) {
        return lval_take(args,i);
      }
    }

    return_val = builtin_op(sym_env, func->sym, args);
  }

  //clean up and return
  lval_del(func);
  return return_val;
}

//======================================
lval* builtin_define(symbol_env* sym_env, lval* args) {
  if( 2 != args->count ) {
    return lval_err("eval accepts exactly 2 args");
  }

  if( LVAL_SYM != args->cell[0]->type ) {
    return lval_err("eval expects 1st arg to be of type LVAL_SYM");
  }

  //TODO error check 'symbol_add's return val

  //set up pointers and free unused memory
  char* symbol = args->cell[0]->sym;
  lval* x = args->cell[1];
  args->cell[0]->sym = NULL;
  args->cell[1] = NULL;
  lval_del(args);
  
  x = eval_lval(sym_env, x);
  unsigned err = symbol_add(sym_env->stack[sym_env->count-1], symbol,x);//TODO remove this dirty hack
  if( 1 == err ) {
    return lval_err("symbol already exists");
  }

  return lval_undef();
}

//======================================
//FUNCTION: what you want to return is already in a list, just label it a qexp
lval* builtin_list(lval* args) {
  args->type = LVAL_QEXP;
  return args;
}

//======================================
lval* builtin_head(lval* args) {
  //check inputs
  if( 1 != args->count ) {
    return lval_err("head accepts exactly 1 arg");
  }

  //TODO check arg is sexp?

  lval* list = lval_take(args,0);
  if( list->count < 1 ) {
    return lval_err("head requires a list of length at least 1");
  }

  //by this point: x is qexp of size > 1 
  lval* head = lval_take(list,0);

  //preserve quoting for nested expressions
  if( LVAL_SEXP == head->type ) {
    head->type = LVAL_QEXP;
  }

  return head;
}

//======================================
lval* builtin_tail(lval* args) {
  //check good state
  if( 1 != args->count ) {
    return lval_err("head accepts exactly 1 arg");
  }

  lval* arg1 = args->cell[0];
  if( !(arg1->count) ) {
    return lval_err("tail requires a list of length at least 1");
  }

  lval* list = lval_take(args,0);
  lval_del(lval_pop(list,0));
  return list;
}

//======================================
lval* builtin_append(lval* args) {
    //get size of next cell
    //realloc to fit both
    //memmove next cell

    lval* qexp = lval_pop(args,0);
    lval* next = NULL;
    while(args->count) {
      next = lval_pop(args,0);
      qexp = lval_join(qexp,next);
    }

    free(args);
    return qexp;
}

//======================================
//TODO add error checking
//TODO use memcpy for efficiency
lval* lval_join(lval* x, lval* y) {
  for(unsigned i = 0; i < y->count; ++i) {
    lval_add(x, y->cell[i]);
  }
  lval_del(y);
  return x;
}

//======================================
lval* builtin_eval(symbol_env* sym_env, lval* args) {
    if( 1 != args->count ) {
      return lval_err("eval accepts exactly 1 arg");
    }

    lval* input = lval_take(args,0);//the rest of ARGS will be freed here
    if( LVAL_QEXP != input->type ) {
      return lval_err("eval expected qexp");
    }

    input->type = LVAL_SEXP;
    return eval_lval(sym_env, input);
}

//======================================
// PURPOSE: dispatch function based on `op`
// NOTE: args will be destroyed
// TODO what if op is not a builtin?
lval* builtin_op(symbol_env* sym_env, char* op, lval* args) {

  if( !strcmp(op,"list") ) {
    return builtin_list(args);
  } else if( !strcmp(op,"head") ){
    return builtin_head(args);
  } else if( !strcmp(op, "tail") ){
    return builtin_tail(args);
  } else if( !strcmp(op, "append") ){
    return builtin_append(args);
  } else if( !strcmp(op, "eval") ){
    return builtin_eval(sym_env, args);
  } else if( !strcmp(op, "lambda") ){
    return builtin_lambda(args);
  }

  //arithmetic
  lval* x = lval_pop(args,0);
  while( args->count ) {
    lval* y = lval_pop(args,0);
    x = apply_op(op,x,y);
  }
  lval_del(args);
  return x;
}

//======================================
lval* builtin_lambda(lval* args) {
  //TODO preconditions

  lval* arg_list = lval_pop(args,0);
  lval* exp = lval_pop(args,0);

  lval* lambda = lval_lambda(arg_list,exp);
  lval_del(args);
  return lambda;
}

//======================================
//look for symbol, if value found, return a copy
lval* eval_symbol(symbol_env* sym_env, lval* x) {
  lval* v = sym_search(sym_env, x->sym);

  if( !v ) {
    lval_del(x);
    return lval_err("symbol not found");
  }
  
  lval_del(x);
  lval* ret;
  lval_copy(v,&ret);
  return ret;
  //return lval_copy(v);
}

//======================================
lval* apply_op(char* op, lval* x, lval* y) {
  if(!strcmp(op,"+")) {
    x->num = x->num + y->num;
  } else if (!strcmp(op,"-")) {
    x->num = x->num - y->num;
  } else if (!strcmp(op,"*")) {
    x->num = x->num * y->num;
  } else if (!strcmp(op,"/")) {
    x->num = x->num / y->num;
  } else {
    return lval_err("invalid symbol");
  }

  return x;
}

/*
********************************************************************************
LVAL UTILS
********************************************************************************
*/
lval* lval_pop(lval* sexp, unsigned index) {
  //#check for correct bounds
  /*
  if( index >= sexp->count ) {
    return lval_err("out of bounds access");
  }
  */

  //#special case of a single element
  /*
  if( 1 == sexp->count) {
    lval* r = sexp->cell[0];
    sexp->cell = NULL;
    return r;
  }
  */

  //#general case
  lval* x = sexp->cell[index];
  //shift
  memmove(&sexp->cell[index],
          &sexp->cell[index+1],
          sizeof(lval*) * (sexp->count - (index + 1)));
  //reflect true size
  sexp->count--;
  sexp->cell = realloc(sexp->cell, sizeof(lval*) * sexp->count);
  return x;
}

//======================================
lval* lval_take(lval* sexp, unsigned index) {
  lval* x = lval_pop(sexp, index);
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
  }
  
  return "UNKNOWN TYPE";
}

/*
********************************************************************************
SYMBOL UTILS
********************************************************************************
*/
//======================================
//add all builtin keywords
unsigned init_symbol_env(symbol_env* sym_env) {
  //TODO error checking
  //TODO refactor this into symbol_env_new_layer() and symbol_env_add_symbol()
  symbol_map* sym_map = make_symbol_map();

  symbol_add(sym_map, strdup("define"), lval_builtin("define"));
  symbol_add(sym_map, strdup("head"),   lval_builtin("head"));
  symbol_add(sym_map, strdup("tail"),   lval_builtin("tail"));
  symbol_add(sym_map, strdup("list"),   lval_builtin("list"));
  symbol_add(sym_map, strdup("join"),   lval_builtin("join"));
  symbol_add(sym_map, strdup("+"),      lval_builtin("+"));
  symbol_add(sym_map, strdup("-"),      lval_builtin("-"));
  symbol_add(sym_map, strdup("*"),      lval_builtin("*"));
  symbol_add(sym_map, strdup("/"),      lval_builtin("/"));
  symbol_add(sym_map, strdup("lambda"), lval_builtin("lambda"));

  sym_env_push(sym_env, sym_map);
  return 0;
}

//======================================
unsigned free_symbol_map(symbol_map* sym_map) {
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
  for(unsigned i = 0; i < sym_env->count; ++i) {
    free_symbol_map(sym_env->stack[i]);
  }

  free(sym_env->stack);
  free(sym_env);
  return 0;
}

//======================================
//WARNING: this expects symbol and value to already be allocated!!!
unsigned symbol_add(symbol_map* sym_map, char* symbol, lval* value) {
  //make sure there is something in the symbol_map before you being searching
  if( sym_map && symbol_find(sym_map, symbol) ) {
    return 1;
  }

  sym_map->count++;
  keyval** new_mem = realloc(sym_map->mem, sizeof(keyval*) * sym_map->count);

  /*
  if( !new_mem ) {
    //TODO have to free the whole table
    free(sym_map->mem);
    return lval_err("memory allocation error");
  }
  */

  sym_map->mem = new_mem;
  sym_map->mem[sym_map->count - 1] = malloc(sizeof(keyval));
  sym_map->mem[sym_map->count - 1]->key = symbol;
  sym_map->mem[sym_map->count - 1]->value = value;
  
  return 0;
}

//======================================
//WARNING: this returns a pointer into SYMBOL_TABLE
lval* symbol_find(symbol_map* sym_map, char* x) {
  //debug
  for(unsigned i = 0; i < sym_map->count; ++i) {
    if( !strcmp(sym_map->mem[i]->key, x) ){
      //printf("value is [%d]\n",sym_map->mem[i]->value->num);
      return sym_map->mem[i]->value;
    }
  }

  return NULL;
}

//======================================
symbol_env* make_env(void) {
  symbol_env* env= malloc(sizeof(symbol_env));
  env->stack = NULL;
  env->count = 0;
  return env;
}

//======================================
symbol_map* make_symbol_map(void) {
  symbol_map* sym_map = malloc(sizeof(symbol_map));
  sym_map->mem = NULL;
  sym_map->count = 0;
  return sym_map;
}

//======================================
// WARNING: this function assumes:
// * arg_list->count == args->count
// * arg_list is a sexp with only LVAL_SYM as children
// * args is a sexp
unsigned populate_symbol_env(symbol_env* sym_env, lval* arg_list, lval* args) {
  //create blank slate;
  symbol_map* sym_map = make_symbol_map();

  //populate
  unsigned count = arg_list->count;
  for(unsigned i = 0; i < count; ++i) {
    push_kv(sym_map, arg_list->cell[i]->sym, args->cell[i]);
  }

  //push
  sym_env_push(sym_env,sym_map);

  return 0;
}

//======================================
//TODO inputs are being destroyed, is this correct?
unsigned push_kv(symbol_map* sym_map, char* key, lval* value) {
  //make room
  sym_map->count++;
  sym_map->mem = realloc(sym_map->mem, sym_map->count);
  //TODO errror check above

  sym_map->mem[sym_map->count - 1] = malloc(sizeof(keyval));
  //TODO errror check above
  keyval* kv = sym_map->mem[sym_map->count - 1];
  kv->key = key;
  kv->value = value;

  return 0;
}

//======================================
unsigned symbol_env_pop(symbol_env* sym_env) {
  sym_env->count--;
  free_symbol_map(sym_env->stack[sym_env->count]);
  sym_env->stack = realloc(sym_env->stack, sym_env->count);

  return 0;
}
