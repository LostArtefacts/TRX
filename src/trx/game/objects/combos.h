#pragma once

#include <trx/game/objects/ids.h>

// Manage the table of object combinations.

// Return the number of objects that combine with an object, and return the
// object at a position.
int32_t ObjectCombo_GetPartnerCount(OBJECT_ID object_id);
OBJECT_ID ObjectCombo_GetPartnerAt(OBJECT_ID object_id, int32_t idx);

// Return the result of combining two objects, or NO_OBJECT if no result
// exists. The order of the objects does not matter.
OBJECT_ID ObjectCombo_GetResult(OBJECT_ID object_id_1, OBJECT_ID object_id_2);
