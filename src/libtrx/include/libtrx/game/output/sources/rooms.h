#pragma once

#include "../../rooms/types.h"
#include "../mesh_batcher/batcher.h"

void OutputSource_Rooms_Init(MESH_BATCHER *batcher);
void OutputSource_Rooms_Shutdown(void);
void OutputSource_Rooms_ObserveLevelLoad(void);
void OutputSource_Rooms_ObserveLevelUnload(void);
void OutputSource_Rooms_ObserveRoomFlip(const ROOM *room);

void OutputSource_Rooms_StageRoom(const ROOM *room);
