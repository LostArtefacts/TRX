#pragma once

#include "global/types.h"

void __cdecl Lara_GetCollisionInfo(ITEM *item, COLL_INFO *coll);

void __cdecl Lara_SlideSlope(ITEM *item, COLL_INFO *coll);

int32_t __cdecl Lara_HitCeiling(ITEM *item, COLL_INFO *coll);

int32_t __cdecl Lara_DeflectEdge(ITEM *item, COLL_INFO *coll);

void __cdecl Lara_DeflectEdgeJump(ITEM *item, COLL_INFO *coll);

void __cdecl Lara_SlideEdgeJump(ITEM *item, COLL_INFO *coll);

int32_t __cdecl Lara_TestWall(
    ITEM *item, int32_t front, int32_t right, int32_t down);

int32_t __cdecl Lara_TestHangOnClimbWall(ITEM *item, COLL_INFO *coll);

int32_t __cdecl Lara_TestClimbStance(ITEM *item, COLL_INFO *coll);

void __cdecl Lara_HangTest(ITEM *item, COLL_INFO *coll);

int32_t __cdecl Lara_TestEdgeCatch(ITEM *item, COLL_INFO *coll, int32_t *edge);

int32_t __cdecl Lara_TestHangJumpUp(ITEM *item, COLL_INFO *coll);

int32_t __cdecl Lara_TestHangJump(ITEM *item, COLL_INFO *coll);

int32_t __cdecl Lara_TestHangSwingIn(ITEM *item, PHD_ANGLE angle);

int32_t __cdecl Lara_TestVault(ITEM *item, COLL_INFO *coll);

int32_t __cdecl Lara_TestSlide(ITEM *item, COLL_INFO *coll);

int16_t __cdecl Lara_FloorFront(ITEM *item, int16_t ang, int32_t dist);

int32_t __cdecl Lara_LandedBad(ITEM *item, COLL_INFO *coll);

int32_t __cdecl Lara_CheckForLetGo(ITEM *item, COLL_INFO *coll);

void __cdecl Lara_GetJointAbsPosition(XYZ_32 *vec, int32_t joint);

void __cdecl Lara_GetJointAbsPosition_I(
    ITEM *item, XYZ_32 *vec, FRAME_INFO *frame1, FRAME_INFO *frame2,
    int32_t frac, int32_t rate);

void __cdecl Lara_BaddieCollision(ITEM *lara_item, COLL_INFO *coll);
void __cdecl Lara_TakeHit(ITEM *lara_item, const COLL_INFO *coll);
void __cdecl Lara_Push(
    const ITEM *item, ITEM *lara_item, COLL_INFO *coll, bool spaz_on,
    bool big_push);
int32_t __cdecl Lara_MovePosition(XYZ_32 *vec, ITEM *item, ITEM *lara_item);
int32_t __cdecl Lara_IsNearItem(const XYZ_32 *pos, int32_t distance);

int32_t __cdecl Lara_TestClimb(
    int32_t x, int32_t y, int32_t z, int32_t x_front, int32_t z_front,
    int32_t item_height, int16_t item_room, int32_t *shift);

int32_t __cdecl Lara_TestClimbPos(
    const ITEM *item, int32_t front, int32_t right, int32_t origin,
    int32_t height, int32_t *shift);

void __cdecl Lara_DoClimbLeftRight(
    ITEM *item, const COLL_INFO *coll, int32_t result, int32_t shift);

int32_t __cdecl Lara_TestClimbUpPos(
    const ITEM *item, int32_t front, int32_t right, int32_t *shift,
    int32_t *ledge);

int32_t __cdecl Lara_GetWaterDepth(
    int32_t x, int32_t y, int32_t z, int16_t room_num);

void __cdecl Lara_TestWaterDepth(ITEM *item, const COLL_INFO *coll);

void __cdecl Lara_SwimCollision(ITEM *item, COLL_INFO *coll);

void __cdecl Lara_WaterCurrent(COLL_INFO *coll);

void __cdecl Lara_CatchFire(void);

void __cdecl Lara_TouchLava(ITEM *item);
