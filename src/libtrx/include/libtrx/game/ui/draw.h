#pragma once

#include "../fader.h"
#include "../output/draw.h"

// Schedule deferred UI draw operations to be executed by UI_Draw().
// These record drawing commands during UI_EndScene instead of issuing them
// immediately.
void UI_ScheduleDrawTextBackground(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h, int32_t z,
    TEXT_STYLE text_style);
void UI_ScheduleDrawTextOutline(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h, int32_t z,
    TEXT_STYLE text_style);
void UI_ScheduleDrawScreenSprite(
    int32_t sx, int32_t sy, int32_t sz, int32_t scale_h, int32_t scale_v,
    int32_t sprite_idx, int16_t shade);
void UI_ScheduleFlush(void);
void UI_ScheduleFaderDraw(FADER *fader);

void UI_InitDraw(void);
void UI_ShutdownDraw(void);
