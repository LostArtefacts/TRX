#pragma once

#include "global/types.h"

void Lara_GetCollisionInfo(ITEM *item, COLL_INFO *coll);

void Lara_SlideSlope(ITEM *item, COLL_INFO *coll);

int32_t Lara_HitCeiling(ITEM *item, COLL_INFO *coll);

int32_t Lara_DeflectEdge(ITEM *item, COLL_INFO *coll);

void Lara_DeflectEdgeJump(ITEM *item, COLL_INFO *coll);

void Lara_SlideEdgeJump(ITEM *item, COLL_INFO *coll);

int32_t Lara_TestWall(ITEM *item, int32_t front, int32_t right, int32_t down);

int32_t Lara_TestHangOnClimbWall(ITEM *item, COLL_INFO *coll);

int32_t Lara_TestClimbStance(ITEM *item, COLL_INFO *coll);

void Lara_HangTest(ITEM *item, COLL_INFO *coll);

int32_t Lara_TestEdgeCatch(ITEM *item, COLL_INFO *coll, int32_t *edge);

int32_t Lara_TestHangJumpUp(ITEM *item, COLL_INFO *coll);

int32_t Lara_TestHangJump(ITEM *item, COLL_INFO *coll);

int32_t Lara_TestHangSwingIn(ITEM *item, int16_t angle);

int32_t Lara_TestVault(ITEM *item, COLL_INFO *coll);

int32_t Lara_TestSlide(ITEM *item, COLL_INFO *coll);

int16_t Lara_FloorFront(ITEM *item, int16_t ang, int32_t dist);

int32_t Lara_LandedBad(ITEM *item, COLL_INFO *coll);

int32_t Lara_CheckForLetGo(ITEM *item, COLL_INFO *coll);

void Lara_GetJointAbsPosition(XYZ_32 *vec, int32_t joint);

void Lara_GetJointAbsPosition_I(
    ITEM *item, XYZ_32 *vec, ANIM_FRAME *frame1, ANIM_FRAME *frame2,
    int32_t frac, int32_t rate);

void Lara_BaddieCollision(ITEM *lara_item, COLL_INFO *coll);
void Lara_TakeHit(ITEM *lara_item, const COLL_INFO *coll);
void Lara_Push(
    const ITEM *item, ITEM *lara_item, COLL_INFO *coll, bool hit_on,
    bool big_push);
int32_t Lara_MovePosition(XYZ_32 *vec, ITEM *item, ITEM *lara_item);
int32_t Lara_IsNearItem(const XYZ_32 *pos, int32_t distance);

int32_t Lara_TestClimb(
    int32_t x, int32_t y, int32_t z, int32_t x_front, int32_t z_front,
    int32_t item_height, int16_t item_room, int32_t *shift);

int32_t Lara_TestClimbPos(
    const ITEM *item, int32_t front, int32_t right, int32_t origin,
    int32_t height, int32_t *shift);

void Lara_DoClimbLeftRight(
    ITEM *item, const COLL_INFO *coll, int32_t result, int32_t shift);

int32_t Lara_TestClimbUpPos(
    const ITEM *item, int32_t front, int32_t right, int32_t *shift,
    int32_t *ledge);

int32_t Lara_GetWaterDepth(int32_t x, int32_t y, int32_t z, int16_t room_num);

void Lara_TestWaterDepth(ITEM *item, const COLL_INFO *coll);

void Lara_SwimCollision(ITEM *item, COLL_INFO *coll);

void Lara_WaterCurrent(COLL_INFO *coll);

void Lara_CatchFire(void);

void Lara_TouchLava(ITEM *item);

// Returns true if Lara has the M16 equipped and is in either anim state: 0
// (start aim); 2 (firing); or 4 (stopping firing).
bool Lara_IsM16Active(void);
