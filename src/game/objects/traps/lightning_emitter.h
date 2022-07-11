#pragma once

#include "global/types.h"

#include <stdint.h>

void LightningEmitter_Setup(OBJECT_INFO *obj);
void LightningEmitter_Initialise(int16_t item_num);
void LightningEmitter_Control(int16_t item_num);
void LightningEmitter_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void LightningEmitter_Draw(ITEM_INFO *item);
