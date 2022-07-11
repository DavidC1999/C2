#include <stdio.h>

#include "hashtable.h"

#define HASHTABLE_SIZE 1000

void greet(char* name) {
    printf("Hello there %s\n", name);
}

void goodbye(char* name) {
    printf("I'll see you later, %s\n", name);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Please supply a name\n");
        return 1;
    }

    HashTable* my_hashtable = hashtable_new(ANY_T, HASHTABLE_SIZE);

    hashtable_set(my_hashtable, "greeter_func", greet);
    hashtable_set(my_hashtable, "goodbye_func", goodbye);

    size_t amt = hashtable_count(my_hashtable);
    printf("The hashtable has %lu entries\n", amt);

    HashEntry greeter_entry, goodbye_entry;
    hashtable_get(my_hashtable, &greeter_entry, "greeter_func");
    hashtable_get(my_hashtable, &goodbye_entry, "goodbye_func");

    void (*greeter_func)(char*) = greeter_entry.value;
    void (*goodbye_func)(char*) = goodbye_entry.value;

    greeter_func(argv[1]);
    goodbye_func(argv[1]);

    hashtable_free(my_hashtable);

    return 0;
}
