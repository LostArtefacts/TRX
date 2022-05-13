#pragma once

// Lara collision helpers.

#include "global/types.h"

#include <stdbool.h>

void Lara_GetCollisionInfo(ITEM_INFO *item, COLL_INFO *coll);
void Lara_HangTest(ITEM_INFO *item, COLL_INFO *coll);
void Lara_SlideSlope(ITEM_INFO *item, COLL_INFO *coll);
bool Lara_Fallen(ITEM_INFO *item, COLL_INFO *coll);
bool Lara_HitCeiling(ITEM_INFO *item, COLL_INFO *coll);
bool Lara_DeflectEdge(ITEM_INFO *item, COLL_INFO *coll);
void Lara_DeflectEdgeJump(ITEM_INFO *item, COLL_INFO *coll);
void Lara_SlideEdgeJump(ITEM_INFO *item, COLL_INFO *coll);
bool Lara_TestVault(ITEM_INFO *item, COLL_INFO *coll);
bool Lara_TestHangJump(ITEM_INFO *item, COLL_INFO *coll);
bool Lara_TestHangJumpUp(ITEM_INFO *item, COLL_INFO *coll);
bool Lara_TestHangSwingIn(ITEM_INFO *item, PHD_ANGLE angle);
bool Lara_TestSlide(ITEM_INFO *item, COLL_INFO *coll);
bool Lara_LandedBad(ITEM_INFO *item, COLL_INFO *coll);
void Lara_SwimCollision(ITEM_INFO *item, COLL_INFO *coll);
void Lara_SurfaceCollision(ITEM_INFO *item, COLL_INFO *coll);
bool Lara_TestWaterClimbOut(ITEM_INFO *item, COLL_INFO *coll);
