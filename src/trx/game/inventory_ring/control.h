#pragma once

#include <trx/game/game_flow/types.h>
#include <trx/game/inventory_ring/types.h>
#include <trx/game/objects/types.h>

INV_RING *InvRing_Open(INVENTORY_MODE mode);
void InvRing_Close(INV_RING *ring);

GF_COMMAND InvRing_Control(INV_RING *ring);
bool InvRing_IsRingAvailable(RING_TYPE ring_type);

void InvRing_AdjustMusicVolume(const INV_RING *ring);
void InvRing_SetRequestedObjectID(OBJECT_ID obj_id);

void InvRing_RemoveAllText(void);

void InvRing_LoadVars(const char *path);
INVENTORY_ITEM *InvRing_GetByObjectID(OBJECT_ID object_id);
