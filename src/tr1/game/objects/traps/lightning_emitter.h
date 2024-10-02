#pragma once

#include "global/types.h"

void LightningEmitter_Setup(OBJECT *obj);
void LightningEmitter_Initialise(int16_t item_num);
void LightningEmitter_Control(int16_t item_num);
void LightningEmitter_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void LightningEmitter_Draw(ITEM *item);
