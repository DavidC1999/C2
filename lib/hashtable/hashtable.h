#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum HashTableType {
    INT_T,
    ANY_T
};

typedef struct HashEntry {
    bool taken;
    char* key;
    void* value;
} HashEntry;

typedef struct HashTable {
    enum HashTableType type;
    size_t size;
    bool free_entry_values;
    HashEntry* entries;
} HashTable;

// specific set method for int
// handles type checking
// returns: success or not
bool hashtable_set_int(HashTable* hashtable, char* key, int64_t value);

// general set method
// type-unsafe
// returns: success or not
bool hashtable_set(HashTable* hashtable, char* key, void* value);

// specific get method for int
// handles type checking
// returns: success or not
bool hashtable_get_int(HashTable* hashtable, int64_t* buffer, char* key);

// general get method
// type-unsafe
// returns: success or not
bool hashtable_get(HashTable* hashtable, HashEntry* buffer, char* key);

// gets the amount of elements in a hashtable
size_t hashtable_count(HashTable* hashtable);

// gets the next entry in the hashtable
// type-unsafe
// returns: True if an item has been written to buffer. False if the latest item was the last one
bool hashtable_get_next(HashTable* hashtable, HashEntry* buffer);

// gets the next int in the hashtable
// type-safe
// returns: True if an item has been written to buffer. False if the latest item was the last one
bool hashtable_get_next_int(HashTable* hashtable, char** buffer_key, int64_t* buffer_value);

// mallocs a new hashtable with initial values
HashTable* hashtable_new(enum HashTableType type, size_t size);

// override default setting
// frees the void* value from all HashEntries in the HashTable on
// a call to hashtable_free
void hashtable_force_free_values(HashTable* hashtable);

// frees the hashtable
void hashtable_free(HashTable* hashtable);

#endif  // _HASHTABLE_H
