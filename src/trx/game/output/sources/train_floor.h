#pragma once

#include <trx/game/output/mesh_batcher/batcher.h>

// The ground and the track beside a moving train, as strips that run along
// the X axis and take the textures of the first animated texture range. Both
// ends of each strip fade into the fog.

void OutputSource_TrainFloor_Init(MESH_BATCHER *batcher);
void OutputSource_TrainFloor_Shutdown(void);
void OutputSource_TrainFloor_ObserveLevelLoad(void);
void OutputSource_TrainFloor_ObserveLevelUnload(void);

// Stages the strips at the current matrix. Does nothing when the level has no
// train ground.
void OutputSource_TrainFloor_Stage(void);
