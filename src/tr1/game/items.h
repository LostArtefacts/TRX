#pragma once

#include "global/types.h"

#include <libtrx/game/items.h>

void Item_Control(void);

bool Item_Test3DRange(int32_t x, int32_t y, int32_t z, int32_t range);

int32_t Item_GetFrames(const ITEM *item, ANIM_FRAME *frmptr[], int32_t *rate);
