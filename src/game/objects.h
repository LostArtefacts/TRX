#pragma once

// Generic collision and draw routines reused between various objects

#include "global/types.h"

#include <stdint.h>

void Object_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void Object_CollisionTrap(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);

void Object_DrawDummyItem(ITEM_INFO *item);
void Object_DrawSpriteItem(ITEM_INFO *item);
void Object_DrawPickupItem(ITEM_INFO *item);
void Object_DrawAnimatingItem(ITEM_INFO *item);
