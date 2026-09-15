#pragma once

#include <trx/core/math/types.h>
#include <trx/game/objects/ids.h>

// Give an object a first frame with these bounds, and mark it loaded, so that
// a measurement of its model has something to read.
void FakeObjects_SetMeshBounds(OBJECT_ID object_id, BOUNDS_16 bounds);
