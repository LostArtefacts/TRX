#include "3dsystem/3d_insert.h"
#include "game/data.h"
#include "mod.h"
#include <math.h>

int MulDiv(int x, int y, int z)
{
    return (x * y) / z;
}

int Tomb1MGetRenderHeightDownscaled()
{
    return PhdWinHeight * PHD_ONE / Tomb1MGetRenderScale(PHD_ONE);
}

int Tomb1MGetRenderWidthDownscaled()
{
    return PhdWinWidth * PHD_ONE / Tomb1MGetRenderScale(PHD_ONE);
}

int Tomb1MGetRenderHeight()
{
    return PhdWinHeight;
}

int Tomb1MGetRenderWidth()
{
    return PhdWinWidth;
}

int Tomb1MGetRenderScale(int unit)
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

int Tomb1MGetRenderScaleGLRage(int unit)
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

void Tomb1MRenderBar(int value, int value_max, int bar_type)
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
    int color_bar[Tomb1M_BAR_NUMBER][COLOR_BAR_SIZE] = {
        { 8, 11, 8, 6, 24 },
        { 32, 41, 32, 19, 21 },
        { 18, 17, 18, 19, 21 },
    };
    if (Tomb1MConfig.enable_red_healthbar) {
        color_bar[Tomb1M_BAR_LARA_HEALTH][0] = 29;
        color_bar[Tomb1M_BAR_LARA_HEALTH][1] = 30;
        color_bar[Tomb1M_BAR_LARA_HEALTH][2] = 29;
        color_bar[Tomb1M_BAR_LARA_HEALTH][3] = 28;
        color_bar[Tomb1M_BAR_LARA_HEALTH][4] = 26;
    }

    const int color_border_1 = 19;
    const int color_border_2 = 17;
    const int color_bgnd = 0;

    int scale = Tomb1MGetRenderScaleGLRage(1);
    int width = percent_max * scale;
    int height = 5 * scale;

    int x = 8 * scale;
    int y = 8 * scale;

    if (bar_type == Tomb1M_BAR_LARA_AIR) {
        // place air bar on the right
        x = PhdWinWidth - width - x;
    } else if (bar_type == Tomb1M_BAR_ENEMY_HEALTH) {
        // place enemy bar on the bottom
        y = PhdWinHeight - height - y;
    }

    int padding = 2;
    int top = y - padding;
    int left = x - padding;
    int bottom = top + height + padding + 1;
    int right = left + width + padding + 1;

    if (bar_type == Tomb1M_BAR_LARA_HEALTH) {
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
    const int blink_threshold = bar_type == Tomb1M_BAR_ENEMY_HEALTH ? 0 : 20;
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
                left, top + i, right, top + i, p4,
                color_bar[bar_type][color_index]);
        }
    }
}

int Tomb1MGetSecretCount()
{
    int count = 0;
    uint32_t secrets = 0;

    for (int i = 0; i < RoomCount; i++) {
        ROOM_INFO* r = &RoomInfo[i];
        FLOOR_INFO* floor = &r->floor[0];
        for (int j = 0; j < r->y_size * r->x_size; j++, floor++) {
            int k = floor->index;
            if (!k) {
                continue;
            }

            while (1) {
                uint16_t floor = FloorData[k++];

                switch (floor & DATA_TYPE) {
                case FT_DOOR:
                case FT_ROOF:
                case FT_TILT:
                    k++;
                    break;

                case FT_LAVA:
                    break;

                case FT_TRIGGER: {
                    uint16_t trig_type = (floor & 0x3F00) >> 8;
                    k++; // skip basic trigger stuff

                    if (trig_type == TT_SWITCH || trig_type == TT_KEY
                        || trig_type == TT_PICKUP) {
                        k++;
                    }

                    while (1) {
                        int16_t command = FloorData[k++];
                        if (TRIG_BITS(command) == TO_CAMERA) {
                            k++;
                        } else if (TRIG_BITS(command) == TO_SECRET) {
                            int16_t number = command & VALUE_BITS;
                            if (!(secrets & (1 << number))) {
                                secrets |= (1 << number);
                                count++;
                            }
                        }

                        if (command & END_BIT) {
                            break;
                        }
                    }
                    break;
                }
                }

                if (floor & END_BIT) {
                    break;
                }
            }
        }
    }

    return count;
}

void Tomb1MFixPyramidSecretTrigger()
{
    if (CurrentLevel != LV_LEVEL10C) {
        return;
    }

    uint32_t global_secrets = 0;

    for (int i = 0; i < RoomCount; i++) {
        uint32_t room_secrets = 0;
        ROOM_INFO* r = &RoomInfo[i];
        FLOOR_INFO* floor = &r->floor[0];
        for (int j = 0; j < r->y_size * r->x_size; j++, floor++) {
            int k = floor->index;
            if (!k) {
                continue;
            }

            while (1) {
                uint16_t floor = FloorData[k++];

                switch (floor & DATA_TYPE) {
                case FT_DOOR:
                case FT_ROOF:
                case FT_TILT:
                    k++;
                    break;

                case FT_LAVA:
                    break;

                case FT_TRIGGER: {
                    uint16_t trig_type = (floor & 0x3F00) >> 8;
                    k++; // skip basic trigger stuff

                    if (trig_type == TT_SWITCH || trig_type == TT_KEY
                        || trig_type == TT_PICKUP) {
                        k++;
                    }

                    while (1) {
                        int16_t* command = &FloorData[k++];
                        if (TRIG_BITS(*command) == TO_CAMERA) {
                            k++;
                        } else if (TRIG_BITS(*command) == TO_SECRET) {
                            int16_t number = *command & VALUE_BITS;
                            if (global_secrets & (1 << number) && number == 0) {
                                // the secret number was already used.
                                // update the number to 2.
                                *command |= 2;
                            } else {
                                room_secrets |= (1 << number);
                            }
                        }

                        if (*command & END_BIT) {
                            break;
                        }
                    }
                    break;
                }
                }

                if (floor & END_BIT) {
                    break;
                }
            }
        }
        global_secrets |= room_secrets;
    }
}
