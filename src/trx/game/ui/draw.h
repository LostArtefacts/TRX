#pragma once

#include <trx/game/output/draw.h>

// Schedule deferred UI draw operations to be executed by UI_Draw().
// These record drawing commands during UI_EndScene instead of issuing them
// immediately.
void UI_ScheduleDrawTextBackground(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h,
    TEXT_STYLE text_style);
void UI_ScheduleDrawTextOutline(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h,
    TEXT_STYLE text_style);
void UI_ScheduleDrawScreenSprite(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int32_t sprite_idx, const RGBA_F colors[4]);
// Draw an already resolved image file in a screen box beneath all other canvas
// content.
void UI_ScheduleDrawImage(
    const char *path, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
    float opacity);
void UI_ScheduleDrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, RGBA_8888 color);
void UI_ScheduleDrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, RGBA_8888 tl,
    RGBA_8888 tr, RGBA_8888 bl, RGBA_8888 br);
void UI_ScheduleDrawHorizontalLine(
    UI_STYLE ui_style, int32_t x0, int32_t x1, int32_t y, int32_t z);
void UI_ScheduleDrawScreenCircle(
    int32_t cx, int32_t cy, int32_t r_inner, int32_t r_outer, int32_t z,
    RGBA_8888 color);

void UI_InitDraw(void);
void UI_ShutdownDraw(void);

// Execute all scheduled UI draw operations in order.
void UI_Draw(void);
