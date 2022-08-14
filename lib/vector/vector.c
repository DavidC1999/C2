#include "vector.h"

#include <stdio.h>

#define PTR_SIZE sizeof(void*)

Vector* vector_new(size_t initial_size) {
    Vector* output = (Vector*)malloc(sizeof(Vector));
    output->arr = (void**)malloc(sizeof(void*) * initial_size);
    output->length = 0;
    output->capacity = initial_size;

    return output;
}

void vector_free(Vector* v) {
    for (size_t i = 0; i < v->length; ++i) {
        free(v->arr[i]);
    }
    free(v->arr);
    free(v);
}

void vector_push(Vector* v, void* elem) {
    if (v->capacity == v->length) {
        v->capacity *= 1.3;

        if (v->capacity == v->length) {  // can happen due to integer rounding
            v->capacity += 1;
        }

        v->arr = realloc(v->arr, sizeof(void*) * v->capacity);
    }

    v->arr[v->length] = elem;
    v->length += 1;
}

void* vector_get(Vector* v, size_t index) {
    return v->arr[index];
}

size_t vector_size(Vector* v) {
    return v->length;
}