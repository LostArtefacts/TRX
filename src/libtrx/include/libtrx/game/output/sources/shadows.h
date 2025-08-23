#pragma once

#include "../mesh_batcher/batcher.h"

void OutputSource_Shadows_Init(MESH_BATCHER *batcher);
void OutputSource_Shadows_Shutdown(void);

void OutputSource_Shadows_StageShadow(void);
