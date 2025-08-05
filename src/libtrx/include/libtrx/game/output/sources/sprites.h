#pragma once

#include "../../rooms/types.h"
#include "../mesh_batcher/batcher.h"
#include "../scene_source.h"

void OutputSource_Sprites_Init(MESH_BATCHER *batcher);
void OutputSource_Sprites_Shutdown(void);
void OutputSource_Sprites_ObserveLevelLoad(void);
void OutputSource_Sprites_ObserveLevelUnload(void);

void OutputSource_Sprites_Stage(int32_t sprite_idx, int16_t shade, RGB_F tint);
