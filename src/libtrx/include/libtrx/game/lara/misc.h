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
extern void Lara_SlideSlope(ITEM *item, COLL_INFO *coll);
extern bool Lara_LandedBad(ITEM *item, COLL_INFO *coll);
extern bool Lara_TestHangJump(ITEM *item, COLL_INFO *coll);
extern void Lara_SlideEdgeJump(ITEM *item, COLL_INFO *coll);
extern void Lara_DeflectEdgeJump(ITEM *item, COLL_INFO *coll);
extern void Lara_HangTest(ITEM *item, COLL_INFO *coll);

// Returns true if Lara has the M16 equipped and is in either anim state: 0
// (start aim); 2 (firing); or 4 (stopping firing).
bool Lara_IsM16Active(void);
