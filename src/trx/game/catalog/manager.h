#pragma once

#include <trx/core/result.h>

#include <stdint.h>

// Separate namespace for each kind of catalog identity.
typedef enum CATALOG_CONTEXT {
    CATALOG_OBJECTS,
    CATALOG_MUSIC,
    CATALOG_SAMPLES,
    CATALOG_LARA_STATES,
    CATALOG_LARA_ANIMS,
    CATALOG_ITEM_ACTIONS,
    CATALOG_WEAPONS,
    CATALOG_CONTEXT_MAX,
} CATALOG_CONTEXT;

typedef int32_t CATALOG_ID;

#define CATALOG_MAX_KEY_SIZE 256
#define NO_CATALOG_ID (-1)

// Built-ins -----------------------------------------------------------------

// Return the executable built-in key: the lower-case C spelling without the
// context prefix. Uses only compile-time data, so it works before catalog
// initialisation. The next call overwrites the result.
const char *Catalog_KeyForEnum(CATALOG_CONTEXT context, const char *enum_name);

// Creation ------------------------------------------------------------------

// Create an identity with no key or slot.
CATALOG_ID Catalog_CreateAnonymous(CATALOG_CONTEXT context);

// Create an identity with a canonical key. Fails if the key is already held.
RESULT Catalog_CreateKey(
    CATALOG_CONTEXT context, const char *key, CATALOG_ID *out_id);

// Create or retrieve the identity for a slot. If the slot is already held,
// returns the identity holding it.
RESULT Catalog_CreateSlot(
    CATALOG_CONTEXT context, int32_t slot, CATALOG_ID *out_id);

// Keys ----------------------------------------------------------------------

// Whether a key is valid: letters, digits, and any of ":_-".
bool Catalog_IsValidKey(const char *key);

// Add an alias for an identity. Fails if the alias is already held.
RESULT Catalog_AddAlias(
    CATALOG_CONTEXT context, CATALOG_ID id, const char *alias);

// Return an identity's canonical key, or NULL if it has none.
const char *Catalog_IDToKey(CATALOG_CONTEXT context, CATALOG_ID id);

// Resolve a canonical key or alias to an identity.
CATALOG_ID Catalog_KeyToID(
    CATALOG_CONTEXT context, const char *key, CATALOG_ID fallback);

// Slots ---------------------------------------------------------------------

// Bind an identity to a slot. Reports failure if another identity already
// holds the slot; the binding is made either way because some contexts
// intentionally share slots.
RESULT Catalog_BindSlot(CATALOG_CONTEXT context, CATALOG_ID id, int32_t slot);

// Bind an identity to the first free slot after all slots currently held by
// the context.
RESULT Catalog_BindFreeSlot(
    CATALOG_CONTEXT context, CATALOG_ID id, int32_t *out_slot);

// Return the slot an identity is bound to, or fallback if it has none.
int32_t Catalog_IDToSlot(
    CATALOG_CONTEXT context, CATALOG_ID id, int32_t fallback);

// Return the identity holding a slot, or fallback if none does. If multiple
// identities share the slot, returns the first.
CATALOG_ID Catalog_SlotToID(
    CATALOG_CONTEXT context, int32_t slot, CATALOG_ID fallback);

// Queries -------------------------------------------------------------------

// Return the number of identities in the context.
int32_t Catalog_GetCount(CATALOG_CONTEXT context);

// Return whether the context holds this identity.
bool Catalog_IsValidID(CATALOG_CONTEXT context, CATALOG_ID id);

// Walk every identity the context holds when the walk starts. One minted
// while it runs is not visited, because it was not there to be walked.
#define CATALOG_FOR_EACH(context, var)                                         \
    for (CATALOG_ID var = 0, var##__end = Catalog_GetCount(context);           \
         var < var##__end; var++)

// Return the number of built-in identities, which is the first position
// available for minted identities.
int32_t Catalog_GetBuiltInCount(CATALOG_CONTEXT context);

// Loading -------------------------------------------------------------------

// Load mappings for a context from a CSV file:
// game_id,name[,comment]
// A game_id of -1 indicates no slot mapping for that entry.
RESULT Catalog_Load(
    CATALOG_CONTEXT context, const char *csv_path, bool allow_duplicates);
