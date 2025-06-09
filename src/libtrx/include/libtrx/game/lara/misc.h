#pragma once

#include "../collision.h"

void Lara_Extinguish(void);
void Lara_TouchLava(void);

int16_t Lara_FloorFront(const ITEM *item, int16_t ang, int32_t dist);
void Lara_GetCollisionInfo(const ITEM *item, COLL_INFO *coll);
extern void Lara_CatchFire(void);

void Lara_UpdateRoomToHeight(int32_t height);
void Lara_ShiftCol(COLL_INFO *coll);
extern bool Lara_TestSlide(ITEM *item, COLL_INFO *coll);
extern bool Lara_HitCeiling(ITEM *item, COLL_INFO *coll);
extern bool Lara_LandedBad(ITEM *item, COLL_INFO *coll);
extern bool Lara_DeflectEdge(ITEM *item, COLL_INFO *coll);
extern bool Lara_TestVault(ITEM *item, COLL_INFO *coll);
extern int32_t Lara_TestHangOnClimbWall(ITEM *item, COLL_INFO *coll);
extern int32_t Lara_TestClimbStance(ITEM *item, COLL_INFO *coll);
int32_t Lara_GetWaterDepth(int32_t x, int32_t y, int32_t z, int16_t room_num);

int32_t Lara_TestClimbPos(
    const ITEM *item, int32_t front, int32_t right, int32_t origin,
    int32_t height, int32_t *shift);

// Returns true if Lara has the M16 equipped and is in either anim state: 0
// (start aim); 2 (firing); or 4 (stopping firing).
bool Lara_IsM16Active(void);
