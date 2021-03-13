#ifndef T1M_GAME_TRAPS_FALLING_BLOCK_H
#define T1M_GAME_TRAPS_FALLING_BLOCK_H

#include "game/types.h"
#include <stdint.h>

void SetupFallingBlock(OBJECT_INFO *obj);
void FallingBlockControl(int16_t item_num);
void FallingBlockFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void FallingBlockCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);

#endif
