// The UI's draw scheduling, doing nothing. A layout test runs the whole scene
// pipeline, and the draw pass is the last of its three phases; discarding what
// it schedules keeps the renderer and its GL dependency out of the tests while
// still exercising measure and layout for real.

#include <trx/game/ui/draw.h>

void UI_InitDraw(void)
{
}

void UI_ShutdownDraw(void)
{
}

void UI_ClearDraw(void)
{
}

void UI_ScheduleDrawScreenSprite(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t scale_h,
    const int32_t scale_v, const int32_t sprite_idx, const RGBA_F colors[4])
{
}

void UI_ScheduleDrawTextBackground(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy,
    const int32_t z, const int32_t w, const int32_t h,
    const TEXT_STYLE text_style)
{
}

void UI_ScheduleDrawTextOutline(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy,
    const int32_t z, const int32_t w, const int32_t h,
    const TEXT_STYLE text_style)
{
}

void UI_ScheduleDrawScreenFlatQuad(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const RGBA_8888 color)
{
}

void UI_ScheduleDrawScreenGradientQuad(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t w,
    const int32_t h, const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
    const RGBA_8888 br)
{
}

void UI_ScheduleDrawHorizontalLine(
    const UI_STYLE ui_style, const int32_t x0, const int32_t x1,
    const int32_t y, const int32_t z)
{
}

void UI_ScheduleDrawScreenCircle(
    const int32_t cx, const int32_t cy, const int32_t r_inner,
    const int32_t r_outer, const int32_t z, const RGBA_8888 color)
{
}
