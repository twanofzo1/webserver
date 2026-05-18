#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "modern_types.h"


#define FILE_MODE_READ "r"
#define FILE_MODE_WRITE "w"
#define FILE_MODE_READ_BINARY "rb"
#define FILE_MODE_WRITE_BINARY "wb"
#define FILE_MODE_APPEND "a"
#define FILE_MODE_APPEND_BINARY "ab"
#define FILE_MODE_READ_WRITE "r+"
#define FILE_MODE_READ_WRITE_BINARY "rb+"
#define FILE_MODE_WRITE_READ "w+"
#define FILE_MODE_WRITE_READ_BINARY "wb+"
#define FILE_MODE_APPEND_READ "a+"
#define FILE_MODE_APPEND_READ_BINARY "ab+"


u64 file_get_size(FILE* file){
    u64 size;
    fseek(file,0,SEEK_END);
    size = ftell(file);
    rewind(file);
    return size;
}


char* file_get_contents(FILE* file, u64* file_size){
    size_t size;
    
    if (file == NULL) {
        return NULL; }

    size = file_get_size(file);
    if (size == 0) {
        return NULL; 
    }

    char* str = (char*)malloc(size + 1);
    if (str == NULL) {
        return NULL; 
    }

    if (fread(str, size, 1, file) != 1) {
        free(str); 
        return NULL;
    }

    str[size] = '\0';
    *file_size = size;
    return str;
}


