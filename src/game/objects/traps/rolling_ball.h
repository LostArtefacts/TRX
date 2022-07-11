#pragma once

#include "global/types.h"

#include <stdint.h>

void RollingBall_Setup(OBJECT_INFO *obj);
void RollingBall_Initialise(int16_t item_num);
void RollingBall_Control(int16_t item_num);
void RollingBall_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
