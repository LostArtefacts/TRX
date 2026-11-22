#pragma once

#include <stdint.h>

// The scenery that passes a moving train: the ground and the track under the
// train, and rows of rocks and cliffs on both sides of it. A level has it when
// the game flow gives it a train ground, and the scenery moves at the UV
// rotate speed.

// Registers the scenery mesh render policy with the object mesh pipeline;
// must run before OutputSource_Objects_ObserveLevelLoad.
void Output_TrainFloor_ObserveLevelLoad(void);
void Output_TrainFloor_ObserveLevelUnload(void);
void Output_TrainFloor_Update(void);

// Reports how far the scenery has moved, and moves it to a position such as
// one read from a save.
int32_t Output_TrainFloor_GetMapPos(void);
void Output_TrainFloor_SetMapPos(int32_t pos);

void Output_TrainFloor_Draw(void);
