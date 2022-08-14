#include <stdio.h>

#include "vector.h"

typedef struct Example {
    int some_value;
    float some_other_value;
} Example;

Example* make_example(int value, float the_other_value) {
    Example* output = malloc(sizeof(Example));

    output->some_value = value;
    output->some_other_value = the_other_value;

    return output;
}

void print_example(Example* ex) {
    printf("Example { some_value = %d, some_other_value = %f }\n", ex->some_value, ex->some_other_value);
}

int main() {
    Vector* my_vec = vector_new(1);  // initial size of 1

    vector_push(my_vec, make_example(10, 0.1));
    vector_push(my_vec, make_example(20, 0.2));
    vector_push(my_vec, make_example(30, 0.3));
    vector_push(my_vec, make_example(40, 0.4));

    for (size_t i = 0; i < vector_size(my_vec); ++i) {
        print_example((Example*)vector_get(my_vec, i));
    }

    vector_free(my_vec);

    return 0;
}