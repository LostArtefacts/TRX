#pragma once

#include "global/types.h"

#include <stdint.h>

void SetupSavegameCrystal(OBJECT_INFO *obj);
void InitialiseSavegameItem(int16_t item_num);
void ControlSavegameItem(int16_t item_num);
void PickUpSavegameCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
