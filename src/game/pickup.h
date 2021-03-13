#ifndef T1M_GAME_PICKUP_H
#define T1M_GAME_PICKUP_H

#include "game/types.h"
#include <stdint.h>

void AnimateLaraUntil(ITEM_INFO *lara_item, int32_t goal);
void PuzzleHoleCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
int32_t KeyTrigger(int16_t item_num);
int32_t PickupTrigger(int16_t item_num);

void T1MInjectGamePickup();

#endif
