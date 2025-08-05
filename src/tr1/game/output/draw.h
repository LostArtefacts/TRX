#pragma once

#include <libtrx/game/objects/types.h>
#include <libtrx/game/output/draw.h>
#include <libtrx/game/rooms/types.h>

void Output_DrawLightningSegment(const LIGHTNING_SEGMENT segment);

void Output_DrawScreenFrame(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 col_dark,
    RGBA_8888 col_light, int32_t thickness);
void Output_DrawScreenGradientBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br, int32_t thickness);
void Output_DrawScreenCentreGradientBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 edge,
    RGBA_8888 center, int32_t thickness);

void Output_DrawSprite(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade, RGB_F tint);
