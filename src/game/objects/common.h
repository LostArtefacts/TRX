#pragma once

// Generic collision and draw routines reused between various objects

#include "global/types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct GAME_OBJECT_PAIR {
    const GAME_OBJECT_ID key_id;
    const GAME_OBJECT_ID value_id;
} GAME_OBJECT_PAIR;

extern const GAME_OBJECT_ID g_EnemyObjects[];
extern const GAME_OBJECT_ID g_PlaceholderObjects[];
extern const GAME_OBJECT_ID g_PickupObjects[];
extern const GAME_OBJECT_ID g_GunObjects[];
extern const GAME_OBJECT_PAIR g_GunAmmoObjectMap[];

bool Object_IsObjectType(
    GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr);
void Object_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void Object_CollisionTrap(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);

void Object_DrawDummyItem(ITEM_INFO *item);
void Object_DrawSpriteItem(ITEM_INFO *item);
void Object_DrawPickupItem(ITEM_INFO *item);
void Object_DrawAnimatingItem(ITEM_INFO *item);
void Object_DrawUnclippedItem(ITEM_INFO *item);
