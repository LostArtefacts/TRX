#pragma once

#include "game/output/mesh_batcher/batcher.h"

#include <libtrx/game/objects/types.h>
#include <libtrx/game/output/scene_source.h>

void OutputSource_Objects_Init(MESH_BATCHER *batcher);
void OutputSource_Objects_Shutdown(void);
void OutputSource_Objects_ObserveLevelLoad(void);
void OutputSource_Objects_ObserveLevelUnload(void);
void OutputSource_Objects_ObserveObjectMeshSwap(
    const OBJECT_MESH *mesh_1, const OBJECT_MESH *mesh_2);
void OutputSource_Objects_ObserveObjectMeshUpdate(const OBJECT_MESH *mesh);

void OutputSource_Objects_StageSkyboxMesh(const OBJECT_MESH *mesh);
void OutputSource_Objects_StageObjectMesh(const OBJECT_MESH *mesh);

const SCENE_SOURCE *OutputSource_Objects_GetSource(void);
