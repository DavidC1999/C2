#include <stdint.h>
#include <stdio.h>

#include "hashtable.h"

#define HASHTABLE_SIZE 1000

int main() {
    HashTable* my_hashtable = hashtable_new(INT_T, HASHTABLE_SIZE);

    hashtable_set_int(my_hashtable, "Apples", 2000);
    hashtable_set_int(my_hashtable, "Oranges", 1000);

    int64_t apples, oranges;
    hashtable_get_int(my_hashtable, &apples, "Apples");
    hashtable_get_int(my_hashtable, &oranges, "Oranges");

    printf("Apples: %ld\nOranges: %ld\n", apples, oranges);

    hashtable_free(my_hashtable);

    return 0;
}
