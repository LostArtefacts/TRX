#pragma once

#include "global/types.h"

#include <stdint.h>

void Lara_Draw(const ITEM *item);
void Lara_Draw_I(
    const ITEM *item, ANIM_FRAME *frame1, ANIM_FRAME *frame2, int32_t frac,
    int32_t rate);
