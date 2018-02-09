#pragma once
#include <stddef.h>

/**************************************/
enum err_sig_t { OK, OUT_OF_MEM, BAD_PARSE };
typedef enum err_sig_t err_sig_t;

struct err_t {
  err_sig_t sig;
};
typedef struct err_t err_t;

/**************************************/
size_t incSize(size_t x);
char* strdup(char* input, err_t* err);
void fucked_up(char* function_name, char* err_msg);
