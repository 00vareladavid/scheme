#include "parse.h"
#include "gen_util.h"
#include "lval_util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

/*******************************************************************************
* GLOBAL
*******************************************************************************/
char* INPUT;

/*******************************************************************************
* Internal
*******************************************************************************/
static unsigned get_file_size(const char* file_name);
static unsigned char* read_whole_file(const char* file_name);
bool whitespace_char(char x);
bool delimiting_char(char x);
void skip_trash(void);
lval_t* parse_bool(char* tok);
lval_t* parse_sym(char* tok, err_t* err);
lval_t* parse_num(char* tok, err_t* err);
lval_t* parse_list(err_t* err);
lval_t* parse_token(char* tok, err_t* err);
void delimit_tok(char* tok);
lval_t* parse_next_lval(err_t* err);

/*
 */
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

/*
 */
bool whitespace_char(char x) {
  return (x == ' ') || (x == '\n');
}

/*
 */
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
lval_t* parse_bool(char* tok) {
  lval_t* x = NULL;

  if ('t' == tok[0]) {
    x = LISP_TRUE;
  } else if ('f' == tok[0]) {
    x = LISP_FALSE;
  } else {
    fucked_up("parse_bool","unknown reader macro");
  }

  if ((tok + 1) != INPUT) {
    fucked_up("parse_bool","2 unknown reader macro");
  }

  return x;
}

/*
 */
lval_t* parse_sym(char* tok, err_t* err) {
  size_t diff = INPUT - tok;
  char* x = malloc(sizeof(char) * (diff + 1));
  memcpy(x, tok, diff);
  x[diff] = '\0';
  lval_t* v = lval_sym(x, err);
  free(x);
  return v;
}

/*
 */
lval_t* parse_num(char* tok, err_t* err) {
  errno = 0;
  char* end;
  int64_t num = strtol(tok, &end, 10);
  if (errno == ERANGE) {
    return lval_err("number out of range", NULL);
  }

  return lval_num(num, err);
}

/*
 */
lval_t* parse_list(err_t* err) {
  lval_t* root = NIL;
  lval_t* x = NULL;
  for (lval_t* next_child = parse_next_lval(err); next_child;
       next_child = parse_next_lval(err)) {
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

/*
 */
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
      return parse_bool(tok + 1);
    case '(':
      return parse_list(err);
    case ')':
      return NULL;
    default: /* has to be an identfier */
      return parse_sym(tok, err);
  }

  return NULL;  // just for the compiler
}

/*
 */
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

/* Purpose: parse a single lval from the INPUT stream
 * Warning: this function should not be called directly
 */
lval_t* parse_next_lval(err_t* err) {
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

/*******************************************************************************
* External
*******************************************************************************/

/*
 */
lval_t* parse_all(char* input, err_t* err) {
  /* set up container */
  lval_t* all = lval_sym("begin", err);
  all = lval_pair(all, NIL, err);

  /* use push+reverse to efficiently read list */
  INPUT = input;
  lval_t* x = parse_next_lval(err);
  while (x) {
    all = ll_push(x, all, err);
    x = parse_next_lval(err);
  }
  all = ll_reverse(all);

  return all;
}

/* Purpose: parse one lval in a stream
 */
lval_t* parse_one(char* input, err_t* err) {
  INPUT = input;
  lval_t* x = parse_next_lval(err);
  free(input);
  return x;
}

/*
 */
lval_t* read_file(char* filename, err_t* err) {
  char* code = (char*)read_whole_file(filename);
  lval_t* x = parse_all(code, err);
  free(code);
  return x;
}
