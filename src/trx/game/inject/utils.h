#pragma once

#include <trx/core/file.h>
#include <trx/game/inject/types.h>
#include <trx/game/objects/types.h>

INJECTION_OBJECT_INFO Inject_ReadObjectPtr(const INJECTION *injection);

// Return the identity bound to a local slot, or NO_CATALOG_ID when the table
// has no symbol for it.
CATALOG_ID Inject_ResolveSymbol(
    const INJECTION *injection, CATALOG_CONTEXT context, int32_t slot);

// Return the object named by an injection. Report failure when the level does
// not bind that slot to an object.
RESULT Inject_GetObject(INJECTION_OBJECT_INFO obj_info, OBJECT **out_obj);
