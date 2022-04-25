#include <stdio.h>
#include <stdlib.h>

int builtin_print(int param) {
    printf("%d\n", param);

    return 0;
}

int builtin_putc(int param) {
    printf("%c", (char)param);

    return 0;
}

int builtin_input_num(int param) {
    (void)param;

    printf(" > ");

    char buffer[11];
    if (fgets(buffer, 11, stdin) != NULL) {
        return atoi(buffer);
    }

    return 0;
}