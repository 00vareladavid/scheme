#include "gen_util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 */
size_t inc_size(size_t x) {
  x += 1u;

  if (!x) {
    fucked_up("inc_size", "unsigned carry(overflow)");
  }

  return x;
}

/*
 */
char* strdup(const char* input) {
  char* x = calloc(inc_size(strlen(input)), sizeof(char));
  if(x) {
    strcpy(x, input);
  }
  return x;
}

/*
 */
char* strdup_e(char* input, err_t* err) {
  char* x = strdup(input);
  if(!x) {
    err->sig = OUT_OF_MEM;
    return NULL;
  }

  return x;
}

/*
 */
void fucked_up(char* function_name, char* err_msg) {
  fprintf(stderr, "[ERROR] %s: %s\n", function_name, err_msg);
  exit(1);
}
