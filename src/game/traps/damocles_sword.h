#pragma once

#include "global/types.h"

void SetupDamoclesSword(OBJECT_INFO *obj);
void InitialiseDamoclesSword(int16_t item_num);
void DamoclesSwordControl(int16_t item_num);
void DamoclesSwordCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
