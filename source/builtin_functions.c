#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "interpreter.h"

int64_t builtin_print(builtin_panic_func_t panic, int64_t count, int64_t* params) {
    if (count != 1) panic("Expected exactly 1 parameter");

    printf("%ld\n", params[0]);

    return 0;
}

int64_t builtin_printu(builtin_panic_func_t panic, int64_t count, int64_t* params) {
    if (count != 1) panic("Expected exactly 1 parameter");

    printf("%lu\n", (uint64_t)(params[0]));

    return 0;
}

int64_t builtin_putc(builtin_panic_func_t panic, int64_t count, int64_t* params) {
    if (count != 1) panic("Expected exactly 1 parameter");

    printf("%c", (char)params[0]);

    return 0;
}

int64_t builtin_puts(builtin_panic_func_t panic, int64_t count, int64_t* params) {
    if (count != 1) panic("Expected exactly 1 parameter");

    char* string = (char*)(params[0]);
    printf("%s\n", string);
    return 0;
}

int64_t builtin_input_num(builtin_panic_func_t panic, int64_t count, int64_t* params) {
    if (count != 0) panic("Expected exactly 0 parameters");
    (void)params;

    printf(" > ");

    char buffer[11];
    if (fgets(buffer, 11, stdin) != NULL) {
        return atoi(buffer);
    }

    return 0;
}