#pragma once

#include <trx/game/catalog/manager.h>

#include <stddef.h>
#include <stdint.h>

#define CATALOG_TABLE_CHUNK 64

typedef struct CATALOG_TABLE {
    CATALOG_CONTEXT context;
    size_t elem_size;
    void **chunks;
    int32_t chunk_count;
    struct CATALOG_TABLE *next;
} CATALOG_TABLE;

#define CATALOG_TABLE_DEFINE(name, ctx, type)                                  \
    static CATALOG_TABLE name;                                                 \
    __attribute__((constructor)) static void M_LinkTable_##name(void)          \
    {                                                                          \
        CatalogTable_Link(&name);                                              \
    }                                                                          \
    static CATALOG_TABLE name = {                                              \
        .context = (ctx),                                                      \
        .elem_size = sizeof(type),                                             \
    }

// Link a table to the catalogue table set and allocate records for the
// identities the context already holds. Table definitions call this before
// main, so constructor order does not matter.
void CatalogTable_Link(CATALOG_TABLE *table);

// Allocate records in every table of a context for the first count identities.
void CatalogTable_Reserve(CATALOG_CONTEXT context, int32_t count);

// Return the zero-initialised record for an ID the context holds. Fails on any
// other ID.
void *CatalogTable_Get(CATALOG_TABLE *table, CATALOG_ID id);

// Return the record for an ID, or null where the context holds no such ID.
// Leaves the table size unchanged.
void *CatalogTable_TryGet(CATALOG_TABLE *table, CATALOG_ID id);

// Return the record for an ID, allocating it where the table has no record.
// Use before catalogue setup when a constructor writes part of a record.
void *CatalogTable_Claim(CATALOG_TABLE *table, CATALOG_ID id);

// Store a record against an ID, allocating it where the table has no record.
// Use before catalogue setup when a constructor registers a routine.
void CatalogTable_Add(CATALOG_TABLE *table, CATALOG_ID id, const void *record);

// Drop minted identity records from every table while retaining built-in
// records initialised by constructors before main.
void CatalogTable_FreeAll(void);

void CatalogTable_Free(CATALOG_TABLE *table);
