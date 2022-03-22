#pragma once

#include "global/types.h"

typedef enum {
    THS_SET = 0,
    THS_TEASE = 1,
    THS_ACTIVE = 2,
    THS_DONE = 3,
} THOR_HAMMER_STATE;

void ThorsHandle_Setup(OBJECT_INFO *obj);
void ThorsHandle_Initialise(int16_t item_num);
void ThorsHandle_Control(int16_t item_num);
void ThorsHandle_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);

void ThorsHead_Setup(OBJECT_INFO *obj);
void ThorsHead_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
