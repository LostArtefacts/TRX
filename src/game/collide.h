#pragma once

#include "global/types.h"

#include <stdint.h>

void GetCollisionInfo(
    COLL_INFO *coll, int32_t xpos, int32_t ypos, int32_t zpos, int16_t room_num,
    int32_t objheight);
int32_t CollideStaticObjects(
    COLL_INFO *coll, int32_t x, int32_t y, int32_t z, int16_t room_number,
    int32_t hite);

void LaraBaddieCollision(ITEM_INFO *lara_item, COLL_INFO *coll);

void ObjectCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void TrapCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
