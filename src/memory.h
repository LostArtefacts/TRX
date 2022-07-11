#pragma once

// Basic memory utilities that exit the game in case the system runs out of
// memory.

#include <stddef.h>

// Allocate n bytes. In case the memory allocation fails, shows an error to the
// user and exits the application. The allocated memory is filled with zeros.
void *Memory_Alloc(size_t size);

// Reallocate existing memory to n bytes, returning an address to the
// reallocated memory. In case the memory allocation fails, shows an error to
// the user and exits the application. All pointers to the old memory address
// become invalid. Preserves the previous memory contents. If the memory is
// NULL, the function acts like Memory_Alloc.
void *Memory_Realloc(void *memory, size_t size);

// Frees the memory associated with a given address. If the memory is NULL, the
// function is a no-op.
void Memory_Free(void *memory);

// Frees the memory associated with a given pointer and sets it to NULL. The
// user is expected to pass a pointer of their variable like so:
//
// char *mem = Memory_Alloc(10);
// Memory_FreePointer(&mem);
// (mem is now NULL)
//
// Giving a NULL to this function is a fatal error. Passing mem directly is
// also an error.
void Memory_FreePointer(void *memory);

// Duplicates a string. In case the memory allocation fails, shows an error to
// the user and exits the application. The string must be NULL-terminated.
// Giving a NULL to this function is a fatal error.
char *Memory_DupStr(const char *string);
