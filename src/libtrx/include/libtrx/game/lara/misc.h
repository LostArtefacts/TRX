#pragma once

#include "../collision.h"
#include "../items/types.h"

void Lara_Extinguish(void);

int16_t Lara_FloorFront(const ITEM *item, int16_t ang, int32_t dist);
void Lara_GetCollisionInfo(const ITEM *item, COLL_INFO *coll);
extern void Lara_CatchFire(void);
