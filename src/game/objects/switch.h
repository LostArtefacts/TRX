#pragma once

#include "global/types.h"

typedef enum {
    SWITCH_STATE_OFF = 0,
    SWITCH_STATE_ON = 1,
    SWITCH_STATE_LINK = 2,
} SWITCH_STATE;

void SetupSwitch1(OBJECT_INFO *obj);
void SetupSwitch2(OBJECT_INFO *obj);
void SwitchCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void SwitchCollisionControlled(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void SwitchCollision2(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void SwitchControl(int16_t item_num);
int32_t SwitchTrigger(int16_t item_num, int16_t timer);
