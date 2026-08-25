#include <trx/game/catalog/table.h>

#include <trx/core/memory.h>

void *CatalogTable_Get(CATALOG_TABLE *const table, const CATALOG_ID id)
{
    if (id < 0) {
        return nullptr;
    }
    if (id < table->builtin_count) {
        return (char *)table->builtin + (size_t)id * table->elem_size;
    }
    const int32_t tail_idx = id - table->builtin_count;
    if (tail_idx >= table->tail_count) {
        return nullptr;
    }
    return table->tail[tail_idx];
}

void *CatalogTable_Append(CATALOG_TABLE *const table)
{
    table->tail_count++;
    table->tail =
        Memory_Realloc(table->tail, sizeof(void *) * table->tail_count);
    void *const record = Memory_Alloc(table->elem_size);
    table->tail[table->tail_count - 1] = record;
    return record;
}

void CatalogTable_Free(CATALOG_TABLE *const table)
{
    for (int32_t i = 0; i < table->tail_count; i++) {
        Memory_FreePointer(&table->tail[i]);
    }
    Memory_FreePointer(&table->tail);
    table->tail_count = 0;
}
