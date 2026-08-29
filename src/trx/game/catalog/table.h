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
    // Mark the table as non-empty so the catalogue drops records keyed by
    // minted identities when the session ends.
    bool linked;
    struct CATALOG_TABLE *next;
} CATALOG_TABLE;

#define CATALOG_TABLE_DEFINE(name, ctx, type)                                  \
    static CATALOG_TABLE name = {                                              \
        .context = (ctx),                                                      \
        .elem_size = sizeof(type),                                             \
    }

// Return the zero-initialised record for an ID, or return null after catalogue
// initialisation if the identity does not exist.
void *CatalogTable_Get(CATALOG_TABLE *table, CATALOG_ID id);

// Drop minted identity records from every table while retaining built-in
// records initialised by constructors before main.
void CatalogTable_FreeAll(void);

void CatalogTable_Free(CATALOG_TABLE *table);
