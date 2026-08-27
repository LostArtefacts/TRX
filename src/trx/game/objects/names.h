#pragma once

#include <trx/core/vector.h>
#include <trx/game/objects/ids.h>

// Get the current name for an object (may change on language reload).
const char *Object_GetName(OBJECT_ID obj_id);

// Every localized name the object answers to, or nullptr if it has none. A
// vector of char*, owned by the name table.
const VECTOR *Object_GetNames(OBJECT_ID obj_id);

// The compile-time English names, nullptr-terminated, or nullptr if the object
// has none. A name lookup falls back on these when the player's language has no
// name to match - which is the case before a language file is loaded at all.
const char *const *Object_GetDefaultNames(OBJECT_ID obj_id);

const char *Object_GetDescription(OBJECT_ID obj_id);

void Object_ResetAllNames(void);
void Object_ClearNames(OBJECT_ID obj_id);
void Object_AddName(OBJECT_ID obj_id, const char *name);

void Object_SetDescription(OBJECT_ID obj_id, const char *description);

// Return the object named by a key, or NO_OBJECT where nothing matches. A key
// is the C spelling without the O_ prefix and in lower case.
OBJECT_ID Object_IdFromKey(const char *key);
