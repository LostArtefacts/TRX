#pragma once

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

void MovableBlock_Setup(OBJECT_INFO *obj);
void MovableBlock_Initialise(int16_t item_num);
void MovableBlock_Control(int16_t item_num);
void MovableBlock_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void MovableBlock_Draw(ITEM_INFO *item);

bool MovableBlock_Test(ITEM_INFO *item, int32_t blockhite);
bool MovableBlock_TestPush(
    ITEM_INFO *item, int32_t blokhite, uint16_t quadrant);
bool MovableBlock_TestPull(
    ITEM_INFO *item, int32_t blokhite, uint16_t quadrant);
