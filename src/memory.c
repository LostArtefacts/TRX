#include "memory.h"

#include "game/shell.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void *Memory_Alloc(size_t size)
{
    void *result = malloc(size);
    if (!result) {
        Shell_ExitSystem("ERROR: Could not allocate enough memory");
    }
    memset(result, 0, size);
    return result;
}

void *Memory_Realloc(void *memory, size_t size)
{
    void *result = realloc(memory, size);
    if (!result) {
        Shell_ExitSystem("ERROR: Could not allocate enough memory");
    }
    return result;
}

void Memory_Free(void *memory)
{
    if (memory) {
        free(memory);
    }
}

void Memory_FreePointer(void *arg)
{
    void *memory;
    memcpy(&memory, arg, sizeof(void *));
    memcpy(arg, &(void *) { NULL }, sizeof(void *));
    Memory_Free(memory);
}

void *Memory_Dup(const char *string)
{
    void *memory = Memory_Alloc(strlen(string) + 1);
    strcpy(memory, string);
    return memory;
}
