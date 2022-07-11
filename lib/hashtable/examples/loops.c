#include <stdint.h>
#include <stdio.h>

#include "hashtable.h"

int main() {
    HashTable* my_hashtable = hashtable_new(INT_T, 20);

    hashtable_set_int(my_hashtable, "a", 0);
    hashtable_set_int(my_hashtable, "b", 1);
    hashtable_set_int(my_hashtable, "c", 2);
    hashtable_set_int(my_hashtable, "d", 3);
    hashtable_set_int(my_hashtable, "e", 4);
    hashtable_set_int(my_hashtable, "f", 5);
    hashtable_set_int(my_hashtable, "g", 6);
    hashtable_set_int(my_hashtable, "h", 7);
    hashtable_set_int(my_hashtable, "h", 8);
    hashtable_set_int(my_hashtable, "h", 9);

    char* key;
    int64_t value;
    while (hashtable_get_next_int(my_hashtable, &key, &value)) {
        printf("%s: %ld\n", key, value);
    }

    return 0;
}