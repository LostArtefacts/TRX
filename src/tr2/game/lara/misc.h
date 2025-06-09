#pragma once

#include "global/types.h"

#include <libtrx/game/lara/misc.h>

void Lara_GetJointAbsPosition(XYZ_32 *vec, int32_t joint);
void Lara_GetJointAbsPosition_I(
    ITEM *item, XYZ_32 *vec, ANIM_FRAME *frame1, ANIM_FRAME *frame2,
    int32_t frac, int32_t rate);
void Lara_BaddieCollision(ITEM *lara_item, COLL_INFO *coll);
void Lara_TakeHit(ITEM *lara_item, const COLL_INFO *coll);
void Lara_WaterCurrent(COLL_INFO *coll);
