#pragma once

#include "../game_flow/types.h"
#include "../objects/types.h"
#include "./types.h"

extern INV_RING *InvRing_Open(INVENTORY_MODE mode);
extern void InvRing_Close(INV_RING *ring);

extern GF_COMMAND InvRing_Control(INV_RING *ring);
extern bool InvRing_IsRingAvailable(RING_TYPE ring_type);

void InvRing_AdjustMusicVolume(const INV_RING *ring);
void InvRing_SetRequestedObjectID(OBJECT_ID obj_id);
