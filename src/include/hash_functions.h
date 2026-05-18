#pragma once
#include "modern_types.h"
#include <string.h>

u64 hash_string(const void* key) {
    const char* str = (const char*)key;
    u64 hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

bool string_equals(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b) == 0;
}