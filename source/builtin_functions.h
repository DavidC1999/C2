#ifndef _BUILTIN_FUNCTIONS_H
#define _BUILTIN_FUNCTIONS_H

#include "interpreter.h"

typedef int64_t (*builtin_func_t)(builtin_panic_func_t, int64_t, int64_t*);

int builtin_print(builtin_panic_func_t panic, int64_t count, int64_t* params);

int builtin_putc(builtin_panic_func_t panic, int64_t count, int64_t* params);

int builtin_input_num(builtin_panic_func_t panic, int64_t count, int64_t* params);

#endif  // _BUILTIN_FUNCTIONS_H