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

// The key an ID answers to, which is what a savegame stores. Null when the
// context holds no such ID.
const char *Catalog_GetKey(CATALOG_CONTEXT context, CATALOG_ID id);

// How many identities the context holds, built-in and minted together.
int32_t Catalog_GetCount(CATALOG_CONTEXT context);

// Bind an identity to this game's ID. Reports failure when another identity
// already holds that game ID; the identity keeps the binding either way.
RESULT Catalog_BindGameID(
    CATALOG_CONTEXT context, CATALOG_ID id, int32_t game_id);

// Load mappings for a specific context from a CSV file of the form:
// game_id,name[,comment]
// A game_id of -1 indicates no mapping for that entry.
// Returns true on success.
RESULT Catalog_Load(
    CATALOG_CONTEXT context, const char *csv_path, bool allow_duplicates);

// Convert an item name to its CATALOG_ID within a context.
// Returns false if not found.
bool Catalog_NameToEnum(
    CATALOG_CONTEXT context, const char *name, CATALOG_ID *out_id);

// Convert a CATALOG_ID to its game-specific ID within a context.
// Returns false if unmapped.
bool Catalog_EnumToGameID(
    CATALOG_CONTEXT context, CATALOG_ID id, int32_t *out_game_id);

// Convert a game-specific ID to its CATALOG_ID within a context.
// Returns false if not found.
bool Catalog_GameIDToEnum(
    CATALOG_CONTEXT context, int32_t game_id, CATALOG_ID *out_id);
