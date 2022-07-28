#include "vector.h"

Vector* vector_new(size_t elem_size, size_t initial_size) {
    Vector* output = (Vector*)malloc(sizeof(Vector));
    output->arr = malloc(elem_size * initial_size);
    output->length = 0;
    output->capacity = initial_size;

    return output;
}

void vector_free(Vector* v) {
    free(v->arr);
    free(v);
}

void vector_push(Vector* v, void* elem) {
    if (v->capacity == v->length) {
        v->capacity *= 1.3;
        v->arr = realloc(v->arr, v->capacity);
    }

    uint8_t* elem_as_byte_arr = (uint8_t*)elem;
    uint8_t* arr_as_byte_arr = (uint8_t*)v->arr;

    for (size_t i = 0; i < v->elem_size; ++i) {
        arr_as_byte_arr[v->elem_size * v->length + i] = elem_as_byte_arr[i];
    }
    v->length += 1;
}

void* vector_get(Vector* v, size_t index) {
    return (uint8_t*)v->arr + (index * v->elem_size);
}

size_t vector_size(Vector* v) {
    return v->length;
}