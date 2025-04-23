#pragma once

#include "global/types.h"

#include <libtrx/game/pathing/lot.h>

#include <stdint.h>

void LOT_InitialiseArray(void);
void LOT_InitialiseSlot(int16_t item_num, int32_t slot);
void LOT_CreateZone(ITEM *item);
void LOT_InitialiseLOT(LOT_INFO *LOT);
