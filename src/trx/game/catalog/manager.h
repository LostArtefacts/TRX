#pragma once

#include <trx/core/result.h>

#include <stdint.h>

// Context discriminator for separate catalog namespaces
typedef enum CATALOG_CONTEXT {
    CATALOG_OBJECTS,
    CATALOG_MUSIC,
    CATALOG_SAMPLES,
    CATALOG_LARA_STATES,
    CATALOG_LARA_ANIMS,
    CATALOG_ITEM_ACTIONS,
    CATALOG_CONTEXT_MAX,
} CATALOG_CONTEXT;

typedef int32_t CATALOG_ID;

// Declare an identity and return its ID. Fails if the context already holds
// the key. The context is the namespace, so the same key in two contexts is
// two identities.
RESULT Catalog_Mint(
    CATALOG_CONTEXT context, const char *key, CATALOG_ID *out_id);

// Give an identity another name to answer to. Fails where the context already
// holds the name. An alias resolves like the canonical key, and is never
// written back out.
RESULT Catalog_AddAlias(
    CATALOG_CONTEXT context, CATALOG_ID id, const char *alias);

// The canonical key an ID answers to, which is what a savegame stores. Null
// when the context holds no such ID.
const char *Catalog_GetKey(CATALOG_CONTEXT context, CATALOG_ID id);

// How many identities the context holds, built-in and minted together.
int32_t Catalog_GetCount(CATALOG_CONTEXT context);

// Bind an identity to the slot this game's files use for it. Reports failure
// when another identity already holds that slot; the binding is made either
// way, because a context such as the samples shares a slot on purpose.
RESULT Catalog_BindSlot(CATALOG_CONTEXT context, CATALOG_ID id, int32_t slot);

// Load mappings for a specific context from a CSV file of the form:
// game_id,name[,comment]
// A game_id of -1 indicates no mapping for that entry.
// Returns true on success.
RESULT Catalog_Load(
    CATALOG_CONTEXT context, const char *csv_path, bool allow_duplicates);

// The identity a key names, or fallback when the context holds no such key.
CATALOG_ID Catalog_FromKey(
    CATALOG_CONTEXT context, const char *key, CATALOG_ID fallback);

// The slot an identity is bound to here, or fallback when it has none.
int32_t Catalog_ToSlot(
    CATALOG_CONTEXT context, CATALOG_ID id, int32_t fallback);

// The identity a slot names here, or fallback when none holds it. A slot that
// two identities share answers with the first of them.
CATALOG_ID Catalog_FromSlot(
    CATALOG_CONTEXT context, int32_t slot, CATALOG_ID fallback);
