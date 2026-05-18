
// define HASHMAP_IMPL

#pragma once
#include "modern_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Hashmap Hashmap;

Hashmap* hashmap_create(u64 capacity, u64(*hash_function)(const void*), bool(*key_equals)(const void*, const void*));
void hashmap_destroy(Hashmap* map);
void* hashmap_set(Hashmap* map, const void* key, void* value);
void* hashmap_get(Hashmap* map, const void* key);
void* hashmap_remove(Hashmap* map, const void* key);
void hashmap_iterate(Hashmap* map, bool(*callback)(const void* key, void* value));
void hashmap_clear(Hashmap* map);


#ifdef HASHMAP_IMPL

struct Hashmap {
    u64(*hash_function)(const void*); // Function pointer for hashing keys
    bool(*key_equals)(const void*, const void*); // Function pointer for comparing keys
    u64 size; // Number of key-value pairs in the hashmap
    u64 capacity; // Total capacity of the hashmap
    void** keys; // Array of pointers to keys
    void** values; // Array of pointers to values
};

Hashmap* hashmap_create(u64 capacity, u64(*hash_function)(const void*), bool(*key_equals)(const void*, const void*)) {
    Hashmap* map = (Hashmap*)malloc(sizeof(Hashmap));
    map->hash_function = hash_function;
    map->key_equals = key_equals;
    map->size = 0;
    map->capacity = capacity;
    map->keys = (void**)malloc(capacity * sizeof(void*));
    map->values = (void**)malloc(capacity * sizeof(void*));
    return map;
}

void hashmap_destroy(Hashmap* map) {
    free(map->keys);
    free(map->values);
    free(map);
}


void* hashmap_set(Hashmap* map, const void* key, void* value) {
    u64 hash = map->hash_function(key);
    u64 index = hash % map->capacity;
    while (map->keys[index] != NULL) {
        if (map->key_equals(map->keys[index], key)) {
            void* old_value = map->values[index];
            map->values[index] = value;
            return old_value; // Return old value if key already exists
        }
        index = (index + 1) % map->capacity; // Linear probing
    }
    map->keys[index] = (void*)key; // Store the key
    map->values[index] = value; // Store the value
    map->size++;
    return NULL; // Return NULL if it's a new key
}

void* hashmap_get(Hashmap* map, const void* key) {
    u64 hash = map->hash_function(key);
    u64 index = hash % map->capacity;
    while (map->keys[index] != NULL) {
        if (map->key_equals(map->keys[index], key)) {
            return map->values[index];
        }
        index = (index + 1) % map->capacity; // Linear probing
    }
    return NULL; // Key not found
}

void* hashmap_remove(Hashmap* map, const void* key) {
    u64 hash = map->hash_function(key);
    u64 index = hash % map->capacity;
    while (map->keys[index] != NULL) {
        if (map->key_equals(map->keys[index], key)) {
            void* old_value = map->values[index];
            map->keys[index] = NULL; // Mark as deleted
            map->values[index] = NULL; // Clear the value
            map->size--;
            return old_value; // Return the removed value
        }
        index = (index + 1) % map->capacity; // Linear probing
    }
    return NULL; // Key not found
}

void hashmap_iterate(Hashmap* map, bool(*callback)(const void* key, void* value)) {
    for (u64 i = 0; i < map->capacity; i++) {
        if (map->keys[i] != NULL) {
            if (!callback(map->keys[i], map->values[i])) {
                break; // Stop iteration if callback returns false
            }
        }
    }
}


void hashmap_clear(Hashmap* map) {
    for (u64 i = 0; i < map->capacity; i++) {
        map->keys[i] = NULL;
        map->values[i] = NULL;
    }
    map->size = 0;
}   

#endif // HASHMAP_IMPL

