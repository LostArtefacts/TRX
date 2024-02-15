#pragma once

#include "global/types.h"

#include <stdint.h>

extern VECTOR_3D g_PuzzleHolePosition;
extern int16_t g_PuzzleHoleBounds[12];

void PuzzleHole_Setup(OBJECT_INFO *obj);
void PuzzleHole_SetupDone(OBJECT_INFO *obj);
void PuzzleHole_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
