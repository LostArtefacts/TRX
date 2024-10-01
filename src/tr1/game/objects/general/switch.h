#pragma once

#include "global/types.h"

typedef enum {
    SWITCH_STATE_OFF = 0,
    SWITCH_STATE_ON = 1,
    SWITCH_STATE_LINK = 2,
} SWITCH_STATE;

void Switch_Setup(OBJECT *obj);
void Switch_SetupUW(OBJECT *obj);
void Switch_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void Switch_CollisionControlled(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void Switch_CollisionUW(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void Switch_Control(int16_t item_num);
bool Switch_Trigger(int16_t item_num, int16_t timer);
