#ifndef T1M_GAME_MOVEBLOCK_H
#define T1M_GAME_MOVEBLOCK_H

#include <stdint.h>

// clang-format off
#define MovableBlockCollision   ((void          (*)(int16_t item_num, ITEM_INFO* litem, COLL_INFO* coll))0x0042B5B0)
#define MovableBlockControl     ((void          (*)(int16_t item_num))0x0042B460)
#define InitialiseRollingBlock  ((void          (*)(int16_t item_num))0x0042BB90)
#define RollingBlockControl     ((void          (*)(int16_t item_num))0x0042BBC0)
#define AlterFloorHeight        ((void          (*)(ITEM_INFO* item, int32_t height))0x0042BCA0)
#define DrawMovableBlock        ((void          (*)(ITEM_INFO *item))0x0042BD60)
// clang-format on

void InitialiseMovableBlock(int16_t item_num);

void T1MInjectGameMoveBlock();

#endif
