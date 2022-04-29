#ifndef _BUILTIN_FUNCTIONS_H
#define _BUILTIN_FUNCTIONS_H

typedef int (*builtin_func_t)(builtin_panic_func_t, int, int*);

int builtin_print(int param);

int builtin_putc(int param);

int builtin_input_num(int param);

#endif  // _BUILTIN_FUNCTIONS_H