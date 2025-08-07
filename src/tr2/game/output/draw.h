#pragma once

#include <libtrx/game/objects/types.h>
#include <libtrx/game/output/draw.h>
#include <libtrx/game/rooms/types.h>

void Output_DrawSprite(
    uint32_t flags, int32_t x, int32_t y, int32_t z, int16_t sprite_idx,
    int16_t shade, int16_t scale);

void Output_DrawScreenFrame(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 col_dark,
    RGBA_8888 col_light, int32_t thickness);
