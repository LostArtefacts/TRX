#ifndef T1M_GAME_OBJECTS_PUZZLE_HOLE_H
#define T1M_GAME_OBJECTS_PUZZLE_HOLE_H

#include "global/types.h"

#include <stdint.h>

extern PHD_VECTOR g_PuzzleHolePosition;
extern int16_t g_PuzzleHoleBounds[12];

void SetupPuzzleHole(OBJECT_INFO *obj);
void SetupPuzzleDone(OBJECT_INFO *obj);
void PuzzleHoleCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
int32_t PickupTrigger(int16_t item_num);

#endif
