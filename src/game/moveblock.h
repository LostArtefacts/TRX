#ifndef T1M_GAME_MOVEBLOCK_H
#define T1M_GAME_MOVEBLOCK_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define InitialiseRollingBlock  ((void          (*)(int16_t item_num))0x0042BB90)
#define RollingBlockControl     ((void          (*)(int16_t item_num))0x0042BBC0)
#define AlterFloorHeight        ((void          (*)(ITEM_INFO* item, int32_t height))0x0042BCA0)
#define DrawMovableBlock        ((void          (*)(ITEM_INFO *item))0x0042BD60)
#define TestBlockPull           ((int32_t       (*)(ITEM_INFO* item, int blokhite, uint16_t quadrant))0x0042B940)
// clang-format on

void InitialiseMovableBlock(int16_t item_num);
void MovableBlockControl(int16_t item_num);
void MovableBlockCollision(
    int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll);
int32_t TestBlockMovable(ITEM_INFO* item, int32_t blockhite);
int32_t TestBlockPush(ITEM_INFO* item, int32_t blokhite, uint16_t quadrant);

void T1MInjectGameMoveBlock();

#endif
