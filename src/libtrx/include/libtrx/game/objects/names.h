#pragma once

#include "ids.h"

#include <stdint.h>

typedef struct {
    GAME_OBJECT_ID object_id;
    const char *matched_name;
} OBJECT_NAME_MATCH;

const char *Object_GetName(GAME_OBJECT_ID obj_id);
const char *Object_GetDescription(GAME_OBJECT_ID obj_id);

void Object_ResetAllNames(void);
void Object_ClearNames(GAME_OBJECT_ID obj_id);
void Object_AddName(GAME_OBJECT_ID obj_id, const char *name);

void Object_SetDescription(GAME_OBJECT_ID obj_id, const char *description);

// Return a list of matching names, with an optional filter callback to only
// consider objects satisfying certain criteria. out_match_count may be
// nullptr. The result must be freed by the caller with Memory_Free().
OBJECT_NAME_MATCH *Object_IdsFromName(
    const char *name, int32_t *out_match_count, bool (*filter)(GAME_OBJECT_ID));

// Return an unique object id for a given programmatic string.
// Example:
//     Given a string "key_1", returns O_KEY_1.
GAME_OBJECT_ID Object_IdFromKey(const char *key);
