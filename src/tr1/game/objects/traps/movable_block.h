#pragma once

#include "global/types.h"

void MovableBlock_Setup(OBJECT *obj);
void MovableBlock_Initialise(int16_t item_num);
void MovableBlock_Control(int16_t item_num);
void MovableBlock_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void MovableBlock_Draw(ITEM *item);
