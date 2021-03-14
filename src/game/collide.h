#ifndef T1M_GAME_COLLIDE_H
#define T1M_GAME_COLLIDE_H

#include "types.h"
#include <stdint.h>

void GetCollisionInfo(
    COLL_INFO *coll, int32_t xpos, int32_t ypos, int32_t zpos, int16_t room_num,
    int32_t objheight);
int32_t FindGridShift(int32_t src, int32_t dst);
int32_t CollideStaticObjects(
    COLL_INFO *coll, int32_t x, int32_t y, int32_t z, int16_t room_number,
    int32_t hite);
void GetNearByRooms(
    int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, int16_t room_num);
void GetNewRoom(int32_t x, int32_t y, int32_t z, int16_t room_num);
void ShiftItem(ITEM_INFO *item, COLL_INFO *coll);
void UpdateLaraRoom(ITEM_INFO *item, int32_t height);
int16_t GetTiltType(FLOOR_INFO *floor, int32_t x, int32_t y, int32_t z);
void LaraBaddieCollision(ITEM_INFO *lara_item, COLL_INFO *coll);
void EffectSpaz(ITEM_INFO *lara_item, COLL_INFO *coll);
void CreatureCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void ObjectCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void DoorCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void TrapCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void ItemPushLara(
    ITEM_INFO *item, ITEM_INFO *lara_item, COLL_INFO *coll, int32_t spazon,
    int32_t bigpush);
int32_t
TestBoundsCollide(ITEM_INFO *item, ITEM_INFO *lara_item, int32_t radius);
int32_t
TestLaraPosition(int16_t *bounds, ITEM_INFO *item, ITEM_INFO *lara_item);
void AlignLaraPosition(PHD_VECTOR *vec, ITEM_INFO *item, ITEM_INFO *lara_item);
int32_t
MoveLaraPosition(PHD_VECTOR *vec, ITEM_INFO *item, ITEM_INFO *lara_item);
int32_t Move3DPosTo3DPos(
    PHD_3DPOS *srcpos, PHD_3DPOS *destpos, int32_t velocity, PHD_ANGLE angadd);
int32_t ItemNearLara(PHD_3DPOS *pos, int32_t distance);

void T1MInjectGameCollide();

#endif
