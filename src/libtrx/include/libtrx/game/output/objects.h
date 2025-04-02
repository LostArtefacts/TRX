#pragma once

#include "../objects/types.h"

#if TR_VERSION == 1
extern void Output_Objects_ObserveMeshSwap(
    const OBJECT_MESH *mesh_1, const OBJECT_MESH *mesh_2);
extern void Output_Objects_ObserveMeshUpdate(const OBJECT_MESH *mesh);
#endif
