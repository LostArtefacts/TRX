#pragma once

#include "../collision.h"

void Lara_Col_Update(ITEM *item, COLL_INFO *coll);

extern void Lara_Col_ClimbStance(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_Climbing(ITEM *item, COLL_INFO *coll);
extern void Lara_Col_ClimbDown(ITEM *item, COLL_INFO *coll);
