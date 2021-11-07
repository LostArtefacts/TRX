#include "dynarray.h"

#include "specific/init.h"

#include <stdlib.h>
#include <string.h>

struct DYNARRAY {
    char *data;
    size_t elem_size;
    size_t used;
    size_t size;
};

DYNARRAY *DynArray_Create(size_t elem_size)
{
    DYNARRAY *arr = malloc(sizeof(DYNARRAY));
    arr->data = NULL;
    arr->size = 0;
    arr->used = 0;
    arr->elem_size = elem_size;
    return arr;
}

size_t DynArray_Size(DYNARRAY *arr)
{
    return arr->used;
}

const void *DynArray_Get(DYNARRAY *arr, size_t idx)
{
    if (idx < 0 || idx >= arr->used) {
        return NULL;
    }
    return arr->data + idx * arr->elem_size;
}

void DynArray_Reset(DYNARRAY *arr)
{
    if (arr->data) {
        free(arr->data);
    }
    arr->data = NULL;
    arr->used = 0;
    arr->size = 0;
}

void DynArray_Append(DYNARRAY *arr, const void *element)
{
    if (arr->used == arr->size) {
        if (!arr->size) {
            arr->size += 1;
        } else {
            arr->size = arr->size * 3 / 2;
        }
        arr->data = realloc(arr->data, arr->size * arr->elem_size);
        if (!arr->data) {
            S_ExitSystem("Failed to allocate memory");
        }
    }
    memcpy(arr->data + arr->used * arr->elem_size, element, arr->elem_size);
    arr->used++;
}

void DynArray_Free(DYNARRAY *arr)
{
    if (arr->data) {
        free(arr->data);
    }
    arr->data = NULL;
    arr->used = 0;
    arr->size = 0;
}
