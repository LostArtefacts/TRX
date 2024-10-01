#pragma once

#include "global/types.h"

void RollingBall_Setup(OBJECT *obj);
void RollingBall_Initialise(int16_t item_num);
void RollingBall_Control(int16_t item_num);
void RollingBall_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
