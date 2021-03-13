#include "3dsystem/3d_insert.h"
#include "game/vars.h"
#include "specific/output.h"
#include "config.h"
#include "util.h"
#include <math.h>

#define COLOR_BAR_SIZE 5

static int8_t color_bar_map[][COLOR_BAR_SIZE] = {
    { 8, 11, 8, 6, 24 }, // gold
    { 32, 41, 32, 19, 21 }, // blue
    { 18, 17, 18, 19, 21 }, // grey
    { 29, 30, 29, 28, 26 }, // red
    { 76, 77, 76, 75, 74 }, // silver
    { 141, 143, 141, 139, 136 }, // green
    { 119, 118, 119, 121, 123 }, // gold2
    { 113, 112, 113, 114, 115 }, // blue2
    { 193, 194, 192, 191, 189 }, // pink
};

int32_t MulDiv(int32_t x, int32_t y, int32_t z)
{
    return (x * y) / z;
}

int32_t GetRenderHeightDownscaled()
{
    return PhdWinHeight * PHD_ONE / GetRenderScale(PHD_ONE);
}

int32_t GetRenderWidthDownscaled()
{
    return PhdWinWidth * PHD_ONE / GetRenderScale(PHD_ONE);
}

int32_t GetRenderHeight()
{
    return PhdWinHeight;
}

int32_t GetRenderWidth()
{
    return PhdWinWidth;
}

int32_t GetRenderScale(int32_t unit)
{
    // TR2Main-style UI scaler
    int32_t baseWidth = 800;
    int32_t baseHeight = 600;
    int32_t scale_x =
        (PhdWinWidth > baseWidth) ? MulDiv(PhdWinWidth, unit, baseWidth) : unit;
    int32_t scale_y = (PhdWinHeight > baseHeight)
        ? MulDiv(PhdWinHeight, unit, baseHeight)
        : unit;
    if (scale_x < scale_y) {
        return scale_x;
    }
    return scale_y;
}

void BarLocation(
    int8_t bar_location, int32_t scale, int32_t width, int32_t height,
    int32_t *x, int32_t *y)
{
    if (bar_location == T1M_BL_TOP_LEFT || bar_location == T1M_BL_BOTTOM_LEFT) {
        *x = 8 * scale;
    } else if (
        bar_location == T1M_BL_TOP_RIGHT
        || bar_location == T1M_BL_BOTTOM_RIGHT) {
        *x = PhdWinWidth - width - 8 * scale;
    } else {
        *x = (PhdWinWidth - width) / 2;
    }

    if (bar_location == T1M_BL_TOP_LEFT || bar_location == T1M_BL_TOP_CENTER
        || bar_location == T1M_BL_TOP_RIGHT) {
        *y = 8 * scale + BarOffsetY[bar_location];
    } else {
        *y = PhdWinHeight - height - 8 * scale - BarOffsetY[bar_location];
    }

    BarOffsetY[bar_location] += height + 4 * scale;
}

void RenderBar(int32_t value, int32_t value_max, int32_t bar_type)
{
    const int32_t p1 = -100;
    const int32_t p2 = -200;
    const int32_t p3 = -400;
    const int32_t percent_max = 100;

    if (value < 0) {
        value = 0;
    } else if (value > value_max) {
        value = value_max;
    }
    int32_t percent = value * 100 / value_max;

    const int32_t color_border_1 = 19;
    const int32_t color_border_2 = 17;
    const int32_t color_bgnd = 0;

    int32_t scale = GetRenderScaleGLRage(1);
    int32_t width = percent_max * scale;
    int32_t height = 5 * scale;
    int16_t bar_color = bar_type;

    int32_t x;
    int32_t y;
    if (bar_type == BT_LARA_HEALTH) {
        BarLocation(T1MConfig.healthbar_location, scale, width, height, &x, &y);
        bar_color = T1MConfig.healthbar_color;
    } else if (bar_type == BT_LARA_AIR) {
        BarLocation(T1MConfig.airbar_location, scale, width, height, &x, &y);
        bar_color = T1MConfig.airbar_color;
    } else if (bar_type == BT_ENEMY_HEALTH) {
        BarLocation(
            T1MConfig.enemy_healthbar_location, scale, width, height, &x, &y);
        bar_color = T1MConfig.enemy_healthbar_color;
    }

    int32_t padding = 2;
    int32_t top = y - padding;
    int32_t left = x - padding;
    int32_t bottom = top + height + padding + 1;
    int32_t right = left + width + padding + 1;

    // background
    for (int32_t i = 1; i < height + 3; i++) {
        Insert2DLine(left + 1, top + i, right, top + i, p1, color_bgnd);
    }

    // top / left border
    Insert2DLine(left, top, right + 1, top, p2, color_border_1);
    Insert2DLine(left, top, left, bottom, p2, color_border_1);

    // bottom / right border
    Insert2DLine(left + 1, bottom, right, bottom, p2, color_border_2);
    Insert2DLine(right, top, right, bottom, p2, color_border_2);

    const int32_t blink_interval = 20;
    const int32_t blink_threshold = bar_type == BT_ENEMY_HEALTH ? 0 : 20;
    int32_t blink_time = Ticks % blink_interval;
    int32_t blink =
        percent <= blink_threshold && blink_time > blink_interval / 2;

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
                color_bar_map[bar_color][color_index]);
        }
    }
}

int32_t GetRenderScaleGLRage(int32_t unit)
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

void S_DrawHealthBar(int32_t percent)
{
    RenderBar(percent, 100, BT_LARA_HEALTH);
}

void S_DrawAirBar(int32_t percent)
{
    RenderBar(percent, 100, BT_LARA_AIR);
}

void T1MInjectSpecificOutput()
{
    INJECT(0x004302D0, S_DrawHealthBar);
    INJECT(0x00430450, S_DrawAirBar);
}
