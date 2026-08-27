#include <trx/game/catalog/table.h>

#include <trx/core/memory.h>

// The tables that hold a tail. A minted identity is dropped when the session
// ends, so the rows keyed by one are dropped with it, and the next session
// does not read what the last one wrote.
static CATALOG_TABLE *m_Tables = nullptr;

void *CatalogTable_Get(CATALOG_TABLE *const table, const CATALOG_ID id)
{
    if (id < 0) {
        return nullptr;
    }
    if (id < table->builtin_count) {
        return (char *)table->builtin + (size_t)id * table->elem_size;
    }
    if (id >= Catalog_GetCount(table->context)) {
        return nullptr;
    }
    // Add rows for minted identities on demand, because most tables hold data
    // for few of them.
    while (id - table->builtin_count >= table->tail_count) {
        CatalogTable_Append(table);
    }
    return table->tail[id - table->builtin_count];
}

void *CatalogTable_Append(CATALOG_TABLE *const table)
{
    if (!table->linked) {
        table->linked = true;
        table->next = m_Tables;
        m_Tables = table;
    }
    table->tail_count++;
    table->tail =
        Memory_Realloc(table->tail, sizeof(void *) * table->tail_count);
    void *const record = Memory_Alloc(table->elem_size);
    table->tail[table->tail_count - 1] = record;
    return record;
}

void CatalogTable_FreeAll(void)
{
    for (CATALOG_TABLE *table = m_Tables; table != nullptr;
         table = table->next) {
        CatalogTable_Free(table);
    }
}

void CatalogTable_Free(CATALOG_TABLE *const table)
{
    for (int32_t i = 0; i < table->tail_count; i++) {
        Memory_FreePointer(&table->tail[i]);
    }
    Memory_FreePointer(&table->tail);
    table->tail_count = 0;
}
