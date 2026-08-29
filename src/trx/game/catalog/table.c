#include <trx/game/catalog/table.h>

#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>

#include <string.h>

static CATALOG_TABLE *m_Tables = nullptr;

static void M_Grow(CATALOG_TABLE *const table, const int32_t count)
{
    const int32_t chunk_count =
        (count + CATALOG_TABLE_CHUNK - 1) / CATALOG_TABLE_CHUNK;
    if (chunk_count <= table->chunk_count) {
        return;
    }
    table->chunks = Memory_Realloc(table->chunks, sizeof(void *) * chunk_count);
    for (int32_t i = table->chunk_count; i < chunk_count; i++) {
        table->chunks[i] = Memory_Alloc(table->elem_size * CATALOG_TABLE_CHUNK);
    }
    table->chunk_count = chunk_count;
}

static void *M_Peek(const CATALOG_TABLE *const table, const CATALOG_ID id)
{
    const int32_t chunk_idx = id / CATALOG_TABLE_CHUNK;
    if (chunk_idx >= table->chunk_count) {
        return nullptr;
    }
    return (char *)table->chunks[chunk_idx]
        + (size_t)(id % CATALOG_TABLE_CHUNK) * table->elem_size;
}

static void *M_Claim(CATALOG_TABLE *const table, const CATALOG_ID id)
{
    ASSERT(id >= 0);
    M_Grow(table, id + 1);
    return M_Peek(table, id);
}

void CatalogTable_Link(CATALOG_TABLE *const table)
{
    table->next = m_Tables;
    m_Tables = table;
    M_Grow(table, Catalog_GetCount(table->context));
}

void CatalogTable_Reserve(const CATALOG_CONTEXT context, const int32_t count)
{
    for (CATALOG_TABLE *table = m_Tables; table != nullptr;
         table = table->next) {
        if (table->context == context) {
            M_Grow(table, count);
        }
    }
}

void *CatalogTable_Get(CATALOG_TABLE *const table, const CATALOG_ID id)
{
    ASSERT(Catalog_IsValidID(table->context, id));
    return M_Claim(table, id);
}

void *CatalogTable_TryGet(CATALOG_TABLE *const table, const CATALOG_ID id)
{
    if (!Catalog_IsValidID(table->context, id)) {
        return nullptr;
    }
    return M_Peek(table, id);
}

void *CatalogTable_Claim(CATALOG_TABLE *const table, const CATALOG_ID id)
{
    return M_Claim(table, id);
}

void CatalogTable_Add(
    CATALOG_TABLE *const table, const CATALOG_ID id, const void *const record)
{
    memcpy(M_Claim(table, id), record, table->elem_size);
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
