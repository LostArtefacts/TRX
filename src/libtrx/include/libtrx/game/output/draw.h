#pragma once

#include "../../config/types.h"
#include "./types.h"

// Fades
extern void Output_DrawBlackRectangle(int32_t opacity);

// UI
extern void Output_DrawTextBackground(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h, int32_t z,
    TEXT_STYLE text_style);
extern void Output_DrawTextOutline(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h, int32_t z,
    TEXT_STYLE text_style);
extern void Output_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t sz, int32_t scale_h, int32_t scale_v,
    int32_t sprite_idx, int16_t shade);
extern void Output_DrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, RGBA_8888 color);
extern void Output_DrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, RGBA_8888 tl,
    RGBA_8888 tr, RGBA_8888 bl, RGBA_8888 br);
