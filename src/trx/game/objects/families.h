#pragma once

#include <trx/game/catalog/manager.h>
#include <trx/game/objects/ids.h>

#include <stdint.h>

// Define independent family memberships that persist across levels with the
// associated object identity.
typedef enum {
#define X_CATALOG_ID(family_id_) family_id_,
#include <trx/game/catalog/families.def>
#undef X_CATALOG_ID
    OBJ_FAMILY_NUMBER_OF,
} OBJECT_FAMILY;

// Return whether an object belongs to a family.
bool ObjectFamily_Has(OBJECT_ID object_id, OBJECT_FAMILY family);

// Add or remove an object from a family; catalogue-created objects use this
// interface because they have no shipped membership data.
void ObjectFamily_Add(OBJECT_ID object_id, OBJECT_FAMILY family);
void ObjectFamily_Remove(OBJECT_ID object_id, OBJECT_FAMILY family);

// Iterate over a family's objects in catalogue order.
#define OBJECT_FAMILY_FOR_EACH(family, var)                                    \
    CATALOG_FOR_EACH(CATALOG_OBJECTS, var)                                     \
    if (ObjectFamily_Has(var, family))
