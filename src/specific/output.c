#include "specific/output.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/3d_insert.h"
#include "game/const.h"
#include "game/vars.h"
#include "game/types.h"
#include "specific/display.h"

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
    int32_t baseWidth = 640;
    int32_t baseHeight = 480;
    int32_t scale_x =
        PhdWinWidth > baseWidth ? MulDiv(PhdWinWidth, unit, baseWidth) : unit;
    int32_t scale_y = PhdWinHeight > baseHeight
        ? MulDiv(PhdWinHeight, unit, baseHeight)
        : unit;
    return MIN(scale_x, scale_y);
}

void BarLocation(
    int8_t bar_location, int32_t width, int32_t height, int32_t *x, int32_t *y)
{
    const int32_t screen_margin_h = 20;
    const int32_t screen_margin_v = 18;
    const int32_t bar_spacing = 8;

    if (bar_location == T1M_BL_TOP_LEFT || bar_location == T1M_BL_BOTTOM_LEFT) {
        *x = screen_margin_h;
    } else if (
        bar_location == T1M_BL_TOP_RIGHT
        || bar_location == T1M_BL_BOTTOM_RIGHT) {
        *x = GetRenderWidthDownscaled() - width - screen_margin_h;
    } else {
        *x = (GetRenderWidthDownscaled() - width) / 2;
    }

    if (bar_location == T1M_BL_TOP_LEFT || bar_location == T1M_BL_TOP_CENTER
        || bar_location == T1M_BL_TOP_RIGHT) {
        *y = screen_margin_v + BarOffsetY[bar_location];
    } else {
        *y = GetRenderHeightDownscaled() - height - screen_margin_v
            - BarOffsetY[bar_location];
    }

    BarOffsetY[bar_location] += height + bar_spacing;
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

    int32_t width = 100;
    int32_t height = 5;
    int16_t bar_color = bar_type;

    int32_t x;
    int32_t y;
    if (bar_type == BT_LARA_HEALTH) {
        BarLocation(T1MConfig.healthbar_location, width, height, &x, &y);
        bar_color = T1MConfig.healthbar_color;
    } else if (bar_type == BT_LARA_AIR) {
        BarLocation(T1MConfig.airbar_location, width, height, &x, &y);
        bar_color = T1MConfig.airbar_color;
    } else if (bar_type == BT_ENEMY_HEALTH) {
        BarLocation(T1MConfig.enemy_healthbar_location, width, height, &x, &y);
        bar_color = T1MConfig.enemy_healthbar_color;
    }

    int32_t padding = 3;
    int32_t left = GetRenderScale(x) - padding;
    int32_t top = GetRenderScale(y) - padding;
    int32_t right = GetRenderScale(x + width) + padding;
    int32_t bottom = GetRenderScale(y + height) + padding;

    // background
    for (int32_t i = top; i < bottom; i++) {
        Insert2DLine(left, i, right, i, p1, color_bgnd);
    }

    // top / left border
    Insert2DLine(left, top, right, top, p2, color_border_1);
    Insert2DLine(left, top, left, bottom, p2, color_border_1);

    // bottom / right border
    Insert2DLine(left, bottom - 1, right, bottom - 1, p2, color_border_2);
    Insert2DLine(right, top, right, bottom, p2, color_border_2);

    const int32_t blink_interval = 20;
    const int32_t blink_threshold = bar_type == BT_ENEMY_HEALTH ? 0 : 20;
    int32_t blink_time = Ticks % blink_interval;
    int32_t blink =
        percent <= blink_threshold && blink_time > blink_interval / 2;

    if (percent && !blink) {
        width = width * percent / percent_max;

        left = GetRenderScale(x);
        top = GetRenderScale(y);
        right = GetRenderScale(x + width);
        bottom = GetRenderScale(y + height);

        for (int i = top; i < bottom; i++) {
            int color_index = (i - top) * COLOR_BAR_SIZE / (bottom - top);
            Insert2DLine(
                left, i, right, i, p3, color_bar_map[bar_color][color_index]);
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

void S_InitialisePolyList()
{
    phd_InitPolyList();
}

void S_InitialiseScreen()
{
    if (CurrentLevel != GF.title_level_num) {
        TempVideoRemove();
    }
}

void S_CalculateLight(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    ROOM_INFO *r = &RoomInfo[room_num];

    if (r->num_lights == 0) {
        LsAdder = r->ambient;
        LsDivider = 0;
    } else {
        int32_t ambient = 0x1FFF - r->ambient;
        int32_t brightest = 0;

        PHD_VECTOR ls;
        for (int i = 0; i < r->num_lights; i++) {
            LIGHT_INFO *light = &r->light[i];
            PHD_VECTOR lc;
            lc.x = x - light->x;
            lc.y = y - light->y;
            lc.z = z - light->z;

            int32_t distance =
                (SQUARE(lc.x) + SQUARE(lc.y) + SQUARE(lc.z)) >> 12;
            int32_t falloff = SQUARE(light->falloff) >> 12;
            int32_t shade =
                ambient + (light->intensity * falloff) / (distance + falloff);

            if (shade > brightest) {
                brightest = shade;
                ls = lc;
            }
        }

        LsAdder = (ambient + brightest) / 2;
        if (brightest == ambient) {
            LsDivider = 0;
        } else {
            LsDivider = (1 << (W2V_SHIFT + 12)) / (brightest - LsAdder);
        }
        LsAdder = 0x1FFF - LsAdder;

        PHD_ANGLE angles[2];
        phd_GetVectorAngles(ls.x, ls.y, ls.z, angles);
        phd_RotateLight(angles[1], angles[0]);
    }

    int32_t distance = PhdMatrixPtr->_23 >> W2V_SHIFT;
    if (distance > DEPTH_Q_START) {
        LsAdder += distance - DEPTH_Q_START;
        if (LsAdder > 0x1FFF) {
            LsAdder = 0x1FFF;
        }
    }
}

void S_CalculateStaticLight(int16_t adder)
{
    LsAdder = adder - 16 * 256;
    int32_t z_dist = PhdMatrixPtr->_23 >> W2V_SHIFT;
    if (z_dist > DEPTH_Q_START) {
        LsAdder += z_dist - DEPTH_Q_START;
    }
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
    INJECT(0x0042FC60, S_InitialisePolyList);
    INJECT(0x0042FCE0, S_InitialiseScreen);
    INJECT(0x00430100, S_CalculateLight);
    INJECT(0x00430290, S_CalculateStaticLight);
    INJECT(0x004302D0, S_DrawHealthBar);
    INJECT(0x00430450, S_DrawAirBar);
}
