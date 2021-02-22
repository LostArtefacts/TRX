#include "3dsystem/3d_insert.h"
#include "game/vars.h"
#include "specific/output.h"
#include "mod.h"
#include "util.h"
#include <math.h>

#define COLOR_BAR_SIZE 5

static int color_bar[][COLOR_BAR_SIZE] = {
    { 8, 11, 8, 6, 24 },
    { 32, 41, 32, 19, 21 },
    { 18, 17, 18, 19, 21 },
};

#ifdef TOMB1M_FEAT_UI
static int BarOffsetY = 0;
#endif

#ifdef TOMB1M_FEAT_UI
int MulDiv(int x, int y, int z)
{
    return (x * y) / z;
}

int GetRenderHeightDownscaled()
{
    return PhdWinHeight * PHD_ONE / GetRenderScale(PHD_ONE);
}

int GetRenderWidthDownscaled()
{
    return PhdWinWidth * PHD_ONE / GetRenderScale(PHD_ONE);
}

int GetRenderHeight()
{
    return PhdWinHeight;
}

int GetRenderWidth()
{
    return PhdWinWidth;
}

int GetRenderScale(int unit)
{
    // TR2Main-style UI scaler
    int baseWidth = 800;
    int baseHeight = 600;
    int scaleX =
        (PhdWinWidth > baseWidth) ? MulDiv(PhdWinWidth, unit, baseWidth) : unit;
    int scaleY = (PhdWinHeight > baseHeight)
        ? MulDiv(PhdWinHeight, unit, baseHeight)
        : unit;
    if (scaleX < scaleY) {
        return scaleX;
    }
    return scaleY;
}

void BarLocation(
    int8_t bar_location, int32_t scale, int32_t width, int32_t height,
    int32_t* x, int32_t* y)
{
    if (bar_location & Tomb1M_BL_HCENTER) {
        *x = (PhdWinWidth - width) / 2;
    } else if (bar_location & Tomb1M_BL_HLEFT) {
        *x = 8 * scale;
    } else if (bar_location & Tomb1M_BL_HRIGHT) {
        *x = PhdWinWidth - width - 8 * scale;
    } else {
        *x = (PhdWinWidth - width) / 2;
    }

    if (bar_location & Tomb1M_BL_VTOP) {
        *y = 8 * scale + BarOffsetY;
    } else if (bar_location & Tomb1M_BL_VBOTTOM) {
        *y = PhdWinHeight - height - 8 * scale - BarOffsetY;
    } else {
        *y = (PhdWinHeight - height) / 2 + BarOffsetY;
    }

    BarOffsetY += height + 4 * scale;
}
#endif

void RenderBar(int value, int value_max, int bar_type)
{
    const int p1 = -100;
    const int p2 = -200;
    const int p3 = -400;
    const int percent_max = 100;

    if (value < 0) {
        value = 0;
    } else if (value > value_max) {
        value = value_max;
    }
    int percent = value * 100 / value_max;

    if (Tomb1MConfig.enable_red_healthbar) {
        color_bar[BT_LARA_HEALTH][0] = 29;
        color_bar[BT_LARA_HEALTH][1] = 30;
        color_bar[BT_LARA_HEALTH][2] = 29;
        color_bar[BT_LARA_HEALTH][3] = 28;
        color_bar[BT_LARA_HEALTH][4] = 26;
    }

    const int color_border_1 = 19;
    const int color_border_2 = 17;
    const int color_bgnd = 0;

    int32_t scale = GetRenderScaleGLRage(1);
    int32_t width = percent_max * scale;
    int32_t height = 5 * scale;

#ifdef TOMB1M_FEAT_UI
    int x;
    int y;
    if (bar_type == BT_LARA_HEALTH) {
        BarOffsetY = 0;
        BarLocation(
            Tomb1MConfig.healthbar_location, scale, width, height, &x, &y);
    } else if (bar_type == BT_LARA_AIR) {
        BarLocation(Tomb1MConfig.airbar_location, scale, width, height, &x, &y);
    } else if (bar_type == BT_ENEMY_HEALTH) {
        BarLocation(
            Tomb1MConfig.enemy_healthbar_location, scale, width, height, &x,
            &y);
    }
#else
    int x = 8 * scale;
    int y = 8 * scale;
    if (bar_type == BT_LARA_AIR) {
        // place air bar on the right
        x = PhdWinWidth - width - x;
    }
#endif

    int padding = 2;
    int top = y - padding;
    int left = x - padding;
    int bottom = top + height + padding + 1;
    int right = left + width + padding + 1;

    if (bar_type == BT_LARA_HEALTH) {
        Tomb1MData.fps_x = left;
        Tomb1MData.fps_y = bottom + 24;
    }

    // background
    for (int i = 1; i < height + 3; i++) {
        Insert2DLine(left + 1, top + i, right, top + i, p1, color_bgnd);
    }

    // top / left border
    Insert2DLine(left, top, right + 1, top, p2, color_border_1);
    Insert2DLine(left, top, left, bottom, p2, color_border_1);

    // bottom / right border
    Insert2DLine(left + 1, bottom, right, bottom, p2, color_border_2);
    Insert2DLine(right, top, right, bottom, p2, color_border_2);

    const int blink_interval = 20;
#ifdef TOMB1M_FEAT_UI
    const int blink_threshold = bar_type == BT_ENEMY_HEALTH ? 0 : 20;
#else
    const int blink_threshold = 20;
#endif
    int blink_time = Ticks % blink_interval;
    int blink = percent <= blink_threshold && blink_time > blink_interval / 2;

    if (percent && !blink) {
        width -= (percent_max - percent) * scale;

        top = y;
        left = x;
        bottom = top + height;
        right = left + width;

        for (int i = 0; i < height; i++) {
            int color_index = i * COLOR_BAR_SIZE / height;
            Insert2DLine(
                left, top + i, right, top + i, p3,
                color_bar[bar_type][color_index]);
        }
    }
}

int GetRenderScaleGLRage(int unit)
{
    // GLRage-style UI scaler
    double result = PhdWinWidth;
    result *= unit;
    result /= 800.0;

    // only scale up, not down
    if (result < unit) {
        result = unit;
    }

    return round(result);
}

void __cdecl S_DrawHealthBar(int32_t percent)
{
    RenderBar(percent, 100, BT_LARA_HEALTH);
}

void __cdecl S_DrawAirBar(int32_t percent)
{
    RenderBar(percent, 100, BT_LARA_AIR);
}

void Tomb1MInjectSpecificOutput()
{
    INJECT(0x004302D0, S_DrawHealthBar);
    INJECT(0x00430450, S_DrawAirBar);
}
