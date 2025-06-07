#pragma once

#include "global/types.h"

#include <libtrx/game/lara/misc.h>

void Lara_HangTest(ITEM *item, COLL_INFO *coll);
bool Lara_Fallen(ITEM *item, COLL_INFO *coll);
bool Lara_HitCeiling(ITEM *item, COLL_INFO *coll);
bool Lara_DeflectEdge(ITEM *item, COLL_INFO *coll);
bool Lara_TestVault(ITEM *item, COLL_INFO *coll);
bool Lara_TestHangJumpUp(ITEM *item, COLL_INFO *coll);
bool Lara_TestHangSwingIn(ITEM *item, PHD_ANGLE angle);
void Lara_SwimCollision(ITEM *item, COLL_INFO *coll);
void Lara_SurfaceCollision(ITEM *item, COLL_INFO *coll);
int32_t Lara_GetWaterDepth(int32_t x, int32_t y, int32_t z, int16_t room_num);
void Lara_TestWaterDepth(ITEM *item, const COLL_INFO *coll);
bool Lara_TestWaterStepOut(ITEM *item, const COLL_INFO *coll);
bool Lara_TestWaterClimbOut(ITEM *item, COLL_INFO *coll);
