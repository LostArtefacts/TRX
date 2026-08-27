#pragma once

#include <trx/game/objects/ids.h>

// Return an object's localized name from "objects/<key>/name", or nullptr if
// none exists.
const char *Object_GetName(OBJECT_ID obj_id);

// Return an object's localized aliases joined with "|", or nullptr if none
// exist. Object lookups match aliases and the primary name.
const char *Object_GetAliases(OBJECT_ID obj_id);

// Return an object's inventory description, or nullptr if none exists.
const char *Object_GetDescription(OBJECT_ID obj_id);

// Return the compile-time English name and aliases. Object lookups use them
// when the player's language has no matching name.
const char *Object_GetDefaultName(OBJECT_ID obj_id);
const char *Object_GetDefaultAliases(OBJECT_ID obj_id);

// Restore the compile-time names as game strings.
void Object_ResetAllNames(void);

// Return the object named by a key, or NO_OBJECT where nothing matches. A key
// is the C spelling without the O_ prefix and in lower case.
OBJECT_ID Object_IdFromKey(const char *key);
