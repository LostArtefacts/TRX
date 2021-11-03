#ifndef T1M_DYNARRAY_H
#define T1M_DYNARRAY_H

#include <stdint.h>

typedef struct DYNARRAY DYNARRAY;

DYNARRAY *DynArray_Create(size_t elem_size);
size_t DynArray_Size(DYNARRAY *arr);
const void *DynArray_Get(DYNARRAY *arr, size_t idx);
void DynArray_Reset(DYNARRAY *arr);
void DynArray_Append(DYNARRAY *arr, const void *element);
void DynArray_Free(DYNARRAY *arr);

#endif
