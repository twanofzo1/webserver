
// define ARENA_IMPL

#pragma once
#include "modern_types.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

typedef struct
{
    u64 reserve_size;
    u64 commit_size;
    u64 current_pos;
    byte *memory;
} Arena;

/// @brief creates an arena struct
/// @param reserve the reserved memory of the arena
/// @param commit the initially commited memory of the arena
/// @return the pointer of the arena
Arena *arena_create(u64 reserve, u64 commit);

/// @brief destroys the arena
/// @param arena the arena to be destroyed
void arena_destroy(Arena *arena);

/// @brief pushes data on the arena
/// @param arena the arena to be pushed to
/// @param size the size of the pushed object
/// @return the pointer to the memory
void *arena_push(Arena *arena, u64 size);

/// @brief pushes an array on the arena
/// @param arena the arena to be pushed to
/// @param type the type of the array
/// @param size the size of the array
#define arena_push_array(arena, type, size) (type*)arena_push(arena, size *sizeof(type))

/// @brief pushes an struct on the arena
/// @param arena the arena to be pushed to
/// @param type the type of the struct
#define arena_push_struct(arena, type) (type*)arena_push(arena, sizeof(type))

/// @brief pops the arena
/// @param arena the arena to be popped
/// @param size the size to be popped
void arena_pop(Arena *arena, u64 size);

/// @brief pops the arena to the specified pos
/// @param arena the arena to be popped
/// @param pos the position to be popped to
void arena_pop_to(Arena *arena, u64 pos);


#ifdef ARENA_IMPL
void *reserve_memory(size_t size)
{   
    #ifdef _WIN32
    void *ptr = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (ptr == NULL)
    {
        perror("failed to reserve memory");
        exit(EXIT_FAILURE);
    }
    return ptr;
    #else
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
    {
        perror("failed to reserve memory");
        exit(EXIT_FAILURE);
    }
    return ptr;
    #endif
}

void commit_memory(byte *reserved_memory, u64 size)
{
    // Touch the pages to commit them
    for (u64 i = 0; i < size; i += 4096)
    {
        reserved_memory[i] = 0;
    }
    // Touch the last byte if size is not a multiple of page size
    if (size > 0)
    {
        reserved_memory[size - 1] = 0;
    }
}

uint64_t next_pow2(uint64_t x)
{
    if (x <= 1) return 1;

    #ifdef _MSC_VER
    unsigned long index;
    if (_BitScanReverse64(&index, x - 1))
        return 1ULL << (index + 1);
    else
        return 1;
    #else
    return 1ULL << (64 - __builtin_clzll(x - 1));
    #endif
}

Arena *arena_create(u64 reserve, u64 commit)
{
    if (commit > reserve){
        perror("commit size cant be larger than reserve size\n");
        exit(EXIT_FAILURE);
    }

    Arena* arena = (Arena *)malloc(sizeof(Arena));
    if (!arena)
    {
        perror("failed to allocate arena\n");
        exit(EXIT_FAILURE);
    }

    u64 commit_size = next_pow2(commit);
    u64 reserve_size = next_pow2(reserve);

    

    arena->memory = (byte *)reserve_memory(reserve_size);
    commit_memory(arena->memory, commit_size);
    arena->current_pos = 0;
    arena->reserve_size = reserve_size;
    arena->commit_size = commit_size;
    return arena;
}

void arena_destroy(Arena *arena)
{
    #ifdef _WIN32
    if (!VirtualFree(arena->memory, 0, MEM_RELEASE))
    {
        perror("arena reserved memory unable to be dealocated\n");
        exit(EXIT_FAILURE);
    }
    #else
    if (munmap(arena->memory, arena->reserve_size) == -1)
    {
        perror("arena reserved memory unable to be dealocated\n");
        exit(EXIT_FAILURE);
    }
    #endif

    free(arena);
    arena = NULL;
}

void *arena_push(Arena *arena, u64 size)
{
    // Align size to the next multiple of 8 for better memory alignment
    size = (size + 7) & ~7;

    // Check if the requested size fits within the committed memory
    if (arena->current_pos + size > arena->commit_size)
    {
        // Calculate the new commit size (e.g., double the current commit size)
        u64 new_commit_size = next_pow2(arena->current_pos + size);

        // Ensure the new commit size does not exceed the reserved size
        if (new_commit_size > arena->reserve_size)
        {
            perror("Requested size exceeds reserved memory\n");
            exit(EXIT_FAILURE);
        }
        // Commit additional memory
        commit_memory(arena->memory + arena->commit_size, new_commit_size - arena->commit_size);
        arena->commit_size = new_commit_size;
    }


    // Allocate memory from the current position
    void *ptr = arena->memory + arena->current_pos;
    arena->current_pos += size;

    return ptr;
}


void arena_pop(Arena *arena, u64 size)
{
    arena->current_pos -= size;
}

void arena_pop_to(Arena *arena, u64 pos)
{
    if (arena->current_pos < pos)
    {
        perror("atempted to pop to an invalid position\n");
        exit(EXIT_FAILURE);
    }
    arena->current_pos = pos;
}
#endif