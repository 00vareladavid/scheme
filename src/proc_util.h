#pragma once
#include "decl.h"
#include <stdint.h>
#include <stdbool.h>

/*
 */
bool extract_arg(lval_t* args, uint16_t num, ...);

/* Purpose: hack to silence compiler warning
 */
bool silence(void* x);
