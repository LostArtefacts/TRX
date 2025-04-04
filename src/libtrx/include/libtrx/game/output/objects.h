#pragma once

#include "../objects/types.h"

#if TR_VERSION == 1
extern void Output_Meshes_ObserveObjectMeshSwap(
    const OBJECT_MESH *mesh_1, const OBJECT_MESH *mesh_2);
extern void Output_Meshes_ObserveObjectMeshUpdate(const OBJECT_MESH *mesh);
#endif
