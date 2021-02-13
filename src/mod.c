#include "game/data.h"
#include "game/shell.h"
#include "mod.h"
#include <math.h>

int TR1MGetOverlayScale(int base)
{
    double result = PhdWinWidth;
    result *= base;
    result /= 800.0;

    // only scale up, not down
    if (result < base) {
        result = base;
    }

    return round(result);
}

void TR1MRenderBar(int value, int value_max, int bar_type)
{
    const int p1 = -100;
    const int p2 = -200;
    const int p3 = -300;
    const int p4 = -400;
    const int percent_max = 100;

    if (value < 0) {
        value = 0;
    } else if (value > value_max) {
        value = value_max;
    }
    int percent = value * 100 / value_max;

#define COLOR_BAR_SIZE 5
    int color_bar[TRM1_BAR_NUMBER][COLOR_BAR_SIZE] = {
        { 8, 11, 8, 6, 24 },
        { 32, 41, 32, 19, 21 },
        { 18, 17, 18, 19, 21 },
    };
    if (TR1MConfig.enable_red_healthbar) {
        color_bar[TRM1_BAR_LARA_HEALTH][0] = 29;
        color_bar[TRM1_BAR_LARA_HEALTH][1] = 30;
        color_bar[TRM1_BAR_LARA_HEALTH][2] = 29;
        color_bar[TRM1_BAR_LARA_HEALTH][3] = 28;
        color_bar[TRM1_BAR_LARA_HEALTH][4] = 26;
    }

    const int color_border_1 = 19;
    const int color_border_2 = 17;
    const int color_bgnd = 0;

    int scale = TR1MGetOverlayScale(1.0);
    int width = percent_max * scale;
    int height = 5 * scale;

    int x = 8 * scale;
    int y = 8 * scale;

    if (bar_type == TRM1_BAR_LARA_AIR) {
        // place air bar on the right
        x = PhdWinWidth - width - x;
    } else if (bar_type == TRM1_BAR_ENEMY_HEALTH) {
        // place enemy bar on the bottom
        y = PhdWinHeight - height - y;
    }

    int padding = 2;
    int top = y - padding;
    int left = x - padding;
    int bottom = top + height + padding + 1;
    int right = left + width + padding + 1;

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
    const int blink_threshold = bar_type == TRM1_BAR_ENEMY_HEALTH ? 0 : 20;
    int blink_time = SaveGame[0].timer % blink_interval;
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
                left, top + i, right, top + i, p4,
                color_bar[bar_type][color_index]);
        }
    }
}
