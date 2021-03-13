#ifndef T1M_GAME_OBJECTS_TRAPDOOR_H
#define T1M_GAME_OBJECTS_TRAPDOOR_H

#include "game/types.h"
#include <stdint.h>

void SetupTrapDoor(OBJECT_INFO *obj);
void TrapDoorControl(int16_t item_num);
void TrapDoorFloor(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
void TrapDoorCeiling(
    ITEM_INFO *item, int32_t x, int32_t y, int32_t z, int16_t *height);
int32_t OnTrapDoor(ITEM_INFO *item, int32_t x, int32_t z);

#endif
