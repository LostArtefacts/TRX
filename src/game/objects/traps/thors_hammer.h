#pragma once

#include "global/types.h"

#include <stdint.h>

void ThorsHandle_Setup(OBJECT_INFO *obj);
void ThorsHandle_Initialise(int16_t item_num);
void ThorsHandle_Control(int16_t item_num);
void ThorsHandle_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);

void ThorsHead_Setup(OBJECT_INFO *obj);
void ThorsHead_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
