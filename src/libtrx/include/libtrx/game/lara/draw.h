#pragma once

#include "../anims/types.h"
#include "../items/types.h"

void Lara_Draw(const ITEM *item);
void Lara_Draw_I(
    const ITEM *item, const ANIM_FRAME *frame1, const ANIM_FRAME *frame2,
    int32_t frac, int32_t rate);
