#ifndef T1M_MEMORY_H
#define T1M_MEMORY_H

// Basic memory utilities that exit the game in case the system runs out of
// memory.

#include <stdint.h>

void *Memory_Alloc(size_t size);
void *Memory_Realloc(void *memory, size_t size);
void Memory_Free(void *memory);

#endif
