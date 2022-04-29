#include <stdio.h>
#include <stdlib.h>

#include "interpreter.h"

int builtin_print(builtin_panic_func_t panic, int count, int* params) {
    if (count != 1) panic("Expected exactly 1 parameter");

    printf("%d\n", params[0]);

    return 0;
}

int builtin_putc(builtin_panic_func_t panic, int count, int* params) {
    if (count != 1) panic("Expected exactly 1 parameter");

    printf("%c", (char)params[0]);

    return 0;
}

int builtin_input_num(builtin_panic_func_t panic, int count, int* params) {
    if (count != 0) panic("Expected exactly 0 parameters");
    (void)params;

    printf(" > ");

    char buffer[11];
    if (fgets(buffer, 11, stdin) != NULL) {
        return atoi(buffer);
    }

    return 0;
}