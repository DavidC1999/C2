#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "vector.h"

int32_t* make_int(int32_t value) {
    printf("Creating random int: %d\n", value);
    int32_t* out = malloc(sizeof(int32_t));
    *out = value;
    return out;
}

int main() {
    srand(time(NULL));

    Vector* my_vec = vector_new(1);  // initial size of 1

    vector_push(my_vec, make_int(rand()));
    vector_push(my_vec, make_int(rand()));
    vector_push(my_vec, make_int(rand()));
    vector_push(my_vec, make_int(rand()));
    vector_push(my_vec, make_int(rand()));
    vector_push(my_vec, make_int(rand()));
    vector_push(my_vec, make_int(rand()));
    vector_push(my_vec, make_int(rand()));
    vector_push(my_vec, make_int(rand()));

    for (size_t i = 0; i < vector_size(my_vec); ++i) {
        printf("%d\n", *(int32_t*)vector_get(my_vec, i));
    }

    vector_free(my_vec);

    return 0;
}