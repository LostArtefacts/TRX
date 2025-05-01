#pragma once

#include "global/types.h"

#include <libtrx/game/items.h>

void Item_Control(void);

int32_t Item_GetFrames(const ITEM *item, ANIM_FRAME *frmptr[], int32_t *rate);
