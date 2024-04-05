#pragma once

#include "global/types.h"

#include <stdint.h>

extern XYZ_32 g_PuzzleHolePosition;

void PuzzleHole_Setup(OBJECT_INFO *obj);
void PuzzleHole_SetupDone(OBJECT_INFO *obj);
void PuzzleHole_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
