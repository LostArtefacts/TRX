#pragma once

#include "global/types.h"

#define SPIKE_DAMAGE 15

void Spikes_Setup(OBJECT_INFO *obj);
void Spikes_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
