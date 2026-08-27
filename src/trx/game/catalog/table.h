#pragma once

#include <trx/game/catalog/manager.h>

#include <stddef.h>
#include <stdint.h>

// Storage for values indexed by a catalog ID. The built-in block is a static
// array sized by the context's sentinel, so a constructor can write to it
// before main. Minted identities live in the tail, which holds one allocation
// per record so that growing the table never moves what is already there.
typedef struct CATALOG_TABLE {
    CATALOG_CONTEXT context;
    size_t elem_size;
    void *builtin;
    int32_t builtin_count;
    void **tail;
    int32_t tail_count;
    // Set once the table grows a tail, so that the catalog can drop every
    // tail when a session ends.
    bool linked;
    struct CATALOG_TABLE *next;
} CATALOG_TABLE;

#define CATALOG_TABLE_DEFINE(name, ctx, type, count)                           \
    static type name##_BuiltIn[count] = {};                                    \
    static CATALOG_TABLE name = {                                              \
        .context = (ctx),                                                      \
        .elem_size = sizeof(type),                                             \
        .builtin = name##_BuiltIn,                                             \
        .builtin_count = (count),                                              \
    }

// Return the record for an ID, or null if the context has no such identity.
// Create a zeroed record on first access to a minted identity.
void *CatalogTable_Get(CATALOG_TABLE *table, CATALOG_ID id);

// Add a record for the next minted identity and return it, zeroed. The
// records already handed out keep their addresses.
void *CatalogTable_Append(CATALOG_TABLE *table);

// Release the tail of every table that grew one. The built-in blocks are
// static and stay.
void CatalogTable_FreeAll(void);

// Release the tail. The built-in block is static and stays.
void CatalogTable_Free(CATALOG_TABLE *table);
