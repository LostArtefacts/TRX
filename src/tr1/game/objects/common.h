#pragma once

// Generic collision and draw routines reused between various objects

#include "global/types.h"

typedef struct {
    const GAME_OBJECT_ID key_id;
    const GAME_OBJECT_ID value_id;
} GAME_OBJECT_PAIR;

GAME_OBJECT_ID Object_GetCognate(
    GAME_OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map);
GAME_OBJECT_ID Object_GetCognateInverse(
    GAME_OBJECT_ID value_id, const GAME_OBJECT_PAIR *test_map);
int16_t Object_FindReceptacle(GAME_OBJECT_ID object_id);
bool Object_IsObjectType(
    GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr);
void Object_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void Object_CollisionTrap(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);

void Object_DrawInterpolatedObject(
    const OBJECT *object, uint32_t meshes, const int16_t *extra_rotation,
    const FRAME_INFO *frame1, const FRAME_INFO *frame2, int32_t frac,
    int32_t rate);
void Object_DrawDummyItem(ITEM *item);
void Object_DrawSpriteItem(ITEM *item);
void Object_DrawPickupItem(ITEM *item);
void Object_DrawAnimatingItem(ITEM *item);
void Object_DrawUnclippedItem(ITEM *item);
void Object_SetMeshReflective(
    GAME_OBJECT_ID object_id, int32_t mesh_idx, bool enabled);
void Object_SetReflective(GAME_OBJECT_ID object_id, bool enabled);
