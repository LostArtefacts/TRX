#include <trx/game/catalog/table.h>

#include <trx/core/memory.h>
#include <trx/core/utils.h>

#include <string.h>

static CATALOG_TABLE *m_Tables = nullptr;

static void *M_At(CATALOG_TABLE *const table, const CATALOG_ID id)
{
    const int32_t chunk_idx = id / CATALOG_TABLE_CHUNK;
    if (chunk_idx >= table->chunk_count) {
        if (!table->linked) {
            table->linked = true;
            table->next = m_Tables;
            m_Tables = table;
        }
        table->chunks =
            Memory_Realloc(table->chunks, sizeof(void *) * (chunk_idx + 1));
        for (int32_t i = table->chunk_count; i <= chunk_idx; i++) {
            table->chunks[i] =
                Memory_Alloc(table->elem_size * CATALOG_TABLE_CHUNK);
        }
        table->chunk_count = chunk_idx + 1;
    }
    return (char *)table->chunks[chunk_idx]
        + (size_t)(id % CATALOG_TABLE_CHUNK) * table->elem_size;
}

void *CatalogTable_Get(CATALOG_TABLE *const table, const CATALOG_ID id)
{
    if (id < 0) {
        return nullptr;
    }
    // Allocate a built-in identity record before catalogue initialisation when
    // a constructor registers its routine.
    const int32_t count = Catalog_GetCount(table->context);
    if (count > 0 && id >= count) {
        return nullptr;
    }
    return M_At(table, id);
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
    const int32_t keep = Catalog_GetBuiltInCount(table->context);
    while (table->chunk_count > 0
           && (table->chunk_count - 1) * CATALOG_TABLE_CHUNK >= keep) {
        Memory_FreePointer(&table->chunks[table->chunk_count - 1]);
        table->chunk_count--;
    }
    if (table->chunk_count == 0) {
        Memory_FreePointer(&table->chunks);
        return;
    }
    // Drop only records after the built-in identities because the final chunk
    // may contain both built-in and minted records.
    const int32_t first = (table->chunk_count - 1) * CATALOG_TABLE_CHUNK;
    for (int32_t id = MAX(first, keep); id < first + CATALOG_TABLE_CHUNK;
         id++) {
        memset(
            (char *)table->chunks[table->chunk_count - 1]
                + (size_t)(id - first) * table->elem_size,
            0, table->elem_size);
    }
}
