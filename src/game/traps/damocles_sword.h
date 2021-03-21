#ifndef T1M_GAME_TRAPS_DAMOCLES_SWORD_H
#define T1M_GAME_TRAPS_DAMOCLES_SWORD_H

#include "global/types.h"

void SetupDamoclesSword(OBJECT_INFO *obj);
void InitialiseDamoclesSword(int16_t item_num);
void DamoclesSwordControl(int16_t item_num);
void DamoclesSwordCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);

#endif
