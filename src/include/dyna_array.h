#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#define DYN_ARRAY(type, name) CREATE_DYNAMIC_ARRAY_TYPE(type, name)

#define CREATE_DYNAMIC_ARRAY_TYPE(type, name)                                      \
    typedef struct {                                                               \
        type* data;                                                                \
        size_t size;                                                               \
        size_t capacity;                                                           \
    } name;                                                                        \
    static inline name name##_init(size_t initial_capacity) {                      \
        name arr;                                                                  \
        assert(initial_capacity > 0 && "Initial capacity must be at least 1");   \
        arr.data = (type*)malloc(initial_capacity * sizeof(type));                 \
        assert(arr.data != NULL && "Memory allocation failed");                  \
        arr.size = 0;                                                              \
        arr.capacity = initial_capacity;                                           \
        return arr;                                                                \
    }                                                                              \
    static inline void name##_push(name* arr, type value) {                        \
        if (arr->size >= arr->capacity) {                                          \
            size_t new_capacity = arr->capacity * 2;                               \
            type* new_data = (type*)realloc(arr->data, new_capacity * sizeof(type));\
            assert(new_data != NULL && "Memory reallocation failed");            \
            arr->data = new_data;                                                  \
            arr->capacity = new_capacity;                                          \
        }                                                                          \
        arr->data[arr->size++] = value;                                            \
    }                                                                              \
    static inline void name##_free(name* arr) {                                    \
        free(arr->data);                                                           \
        arr->data = NULL;                                                          \
        arr->size = 0;                                                             \
        arr->capacity = 0;                                                         \
     }
