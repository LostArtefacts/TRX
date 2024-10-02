#pragma once

#include "global/types.h"

#include <stdint.h>

void Lara_Draw(ITEM *item);
void Lara_Draw_I(
    ITEM *item, FRAME_INFO *frame1, FRAME_INFO *frame2, int32_t frac,
    int32_t rate);
