#ifndef _VECTOR_H
#define _VECTOR_H

#include <stdint.h>
#include <stdlib.h>

typedef struct Vector {
    void** arr;       // array of void*
    size_t length;    // amount of elements stored
    size_t capacity;  // amount of elements able to be stored at this point in time
} Vector;

Vector* vector_new(size_t initial_size);

void vector_free(Vector* v);

// copies 'elem', does not take ownership
void vector_push(Vector* v, void* elem);

void* vector_get(Vector* v, size_t index);

size_t vector_size(Vector* v);

#endif  //_VECTOR_H