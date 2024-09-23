#pragma once

#include "global/types.h"

void DamoclesSword_Setup(OBJECT *obj);
void DamoclesSword_Initialise(int16_t item_num);
void DamoclesSword_Control(int16_t item_num);
void DamoclesSword_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
