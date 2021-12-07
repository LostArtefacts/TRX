#pragma once

#include "global/types.h"

#define SPIKE_DAMAGE 15

void SetupSpikes(OBJECT_INFO *obj);
void SpikeCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
