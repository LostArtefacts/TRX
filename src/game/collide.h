#pragma once

#include "global/types.h"

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
void TrapCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
bool Move3DPosTo3DPos(
    PHD_3DPOS *srcpos, PHD_3DPOS *destpos, int32_t velocity, int16_t rotation,
    ITEM_INFO *lara_item);
