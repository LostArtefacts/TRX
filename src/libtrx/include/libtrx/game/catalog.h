#pragma once

#include "uuid.h"

#include <stdbool.h>
#include <stdint.h>

// Context discriminator for separate catalog namespaces
typedef enum CATALOG_CONTEXT {
    CATALOG_OBJECTS,
    CATALOG_MUSIC,
    CATALOG_SAMPLES,
    CATALOG_CONTEXT_MAX,
} CATALOG_CONTEXT;

typedef int32_t CATALOG_ID;

// Load mappings for a specific context from a CSV file of the form:
// uuid,game_id[,comment]
// A game_id of -1 indicates no mapping for that entry.
// Returns true on success.
bool Catalog_Load(CATALOG_CONTEXT context, const char *csv_path);

// Convert a UUID to its CATALOG_ID within a context.
// Returns false if not found.
bool Catalog_UUIDToEnum(CATALOG_CONTEXT context, UUID uuid, CATALOG_ID *out_id);

// Convert a CATALOG_ID to its UUID within a context.
// Returns nullptr if invalid.
const UUID *Catalog_EnumToUUID(CATALOG_CONTEXT context, CATALOG_ID id);

// Convert a CATALOG_ID to its game-specific ID within a context.
// Returns false if unmapped.
bool Catalog_EnumToGameID(
    CATALOG_CONTEXT context, CATALOG_ID id, int32_t *out_game_id);

// Convert a game-specific ID to its CATALOG_ID within a context.
// Returns false if not found.
bool Catalog_GameIDToEnum(
    CATALOG_CONTEXT context, int32_t game_id, CATALOG_ID *out_id);

// Free internal resources.
void Catalog_Shutdown(void);
