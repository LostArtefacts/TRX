#pragma once

#include <libtrx/game/game_flow/types.h>
#include <libtrx/game/inventory_ring/control.h>
#include <libtrx/game/inventory_ring/enum.h>
#include <libtrx/game/inventory_ring/types.h>
#include <libtrx/game/objects/types.h>

INV_RING *InvRing_Open(INVENTORY_MODE mode);
GF_COMMAND InvRing_Control(INV_RING *ring, int32_t num_frames);
void InvRing_Close(INV_RING *ring);

void InvRing_RemoveAllText(void);
