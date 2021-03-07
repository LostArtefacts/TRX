#ifndef T1M_GAME_MOVEBLOCK_H
#define T1M_GAME_MOVEBLOCK_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
// clang-format on

void InitialiseMovableBlock(int16_t item_num);
void MovableBlockControl(int16_t item_num);
void MovableBlockCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
int32_t TestBlockMovable(ITEM_INFO *item, int32_t blockhite);
int32_t TestBlockPush(ITEM_INFO *item, int32_t blokhite, uint16_t quadrant);
int32_t TestBlockPull(ITEM_INFO *item, int32_t blokhite, uint16_t quadrant);

void InitialiseRollingBlock(int16_t item_num);
void RollingBlockControl(int16_t item_num);

void AlterFloorHeight(ITEM_INFO *item, int32_t height);

void DrawMovableBlock(ITEM_INFO *item);

void T1MInjectGameMoveBlock();

#endif
