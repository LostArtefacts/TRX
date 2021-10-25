#include "specific/output.h"

#include "3dsystem/3d_gen.h"
#include "config.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/clock.h"
#include "specific/display.h"
#include "specific/file.h"
#include "specific/frontend.h"
#include "specific/hwr.h"
#include "specific/smain.h"
#include "util.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define COLOR_STEPS 5
#define VISIBLE(vn1, vn2, vn3)                                                                            \
    ((int32_t)(((vn3->xs - vn2->xs) * (vn1->ys - vn2->ys)) - ((vn1->xs - vn2->xs) * (vn3->ys - vn2->ys))) \
     >= 0)

static RGB888 ColorBarMap[][COLOR_STEPS] = {
    // gold
    { { 112, 92, 44 },
      { 164, 120, 72 },
      { 112, 92, 44 },
      { 88, 68, 0 },
      { 80, 48, 20 } },
    // blue
    { { 100, 116, 100 },
      { 92, 160, 156 },
      { 100, 116, 100 },
      { 76, 80, 76 },
      { 48, 48, 48 } },
    // grey
    { { 88, 100, 88 },
      { 116, 132, 116 },
      { 88, 100, 88 },
      { 76, 80, 76 },
      { 48, 48, 48 } },
    // red
    { { 160, 40, 28 },
      { 184, 44, 32 },
      { 160, 40, 28 },
      { 124, 32, 32 },
      { 84, 20, 32 } },
    // silver
    { { 150, 150, 150 },
      { 230, 230, 230 },
      { 200, 200, 200 },
      { 140, 140, 140 },
      { 100, 100, 100 } },
    // green
    { { 100, 190, 20 },
      { 130, 230, 30 },
      { 100, 190, 20 },
      { 90, 150, 15 },
      { 80, 110, 10 } },
    // gold2
    { { 220, 170, 0 },
      { 255, 200, 0 },
      { 220, 170, 0 },
      { 185, 140, 0 },
      { 150, 100, 0 } },
    // blue2
    { { 0, 170, 220 },
      { 0, 200, 255 },
      { 0, 170, 220 },
      { 0, 140, 185 },
      { 0, 100, 150 } },
    // pink
    { { 220, 140, 170 },
      { 255, 150, 200 },
      { 210, 130, 160 },
      { 165, 100, 120 },
      { 120, 60, 70 } },
};

static int DecompPCX(const char *pcx, size_t pcx_size, char *pic, RGB888 *pal);

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
    int32_t baseWidth = 640;
    int32_t baseHeight = 480;
    int32_t scale_x = PhdWinWidth > baseWidth
        ? ((double)PhdWinWidth * unit * UITextScale) / baseWidth
        : unit * UITextScale;
    int32_t scale_y = PhdWinHeight > baseHeight
        ? ((double)PhdWinHeight * unit * UITextScale) / baseHeight
        : unit * UITextScale;
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
        *x = GetRenderWidthDownscaled() - width * UIBarScale - screen_margin_h;
    } else {
        *x = (GetRenderWidthDownscaled() - width) / 2;
    }

    if (bar_location == T1M_BL_TOP_LEFT || bar_location == T1M_BL_TOP_CENTER
        || bar_location == T1M_BL_TOP_RIGHT) {
        *y = screen_margin_v + BarOffsetY[bar_location];
    } else {
        *y = GetRenderHeightDownscaled() - height * UIBarScale - screen_margin_v
            - BarOffsetY[bar_location];
    }

    BarOffsetY[bar_location] += height + bar_spacing;
}

void RenderBar(int32_t value, int32_t value_max, int32_t bar_type)
{
    static int32_t blink_counter = 0;
    const int32_t percent_max = 100;

    if (value < 0) {
        value = 0;
    } else if (value > value_max) {
        value = value_max;
    }
    int32_t percent = value * 100 / value_max;

    const RGB888 rgb_bgnd = { 0, 0, 0 };
    const RGB888 rgb_border_light = { 128, 128, 128 };
    const RGB888 rgb_border_dark = { 64, 64, 64 };

    int32_t width = 100;
    int32_t height = 5;
    int16_t bar_color = bar_type;

    int32_t x = 0;
    int32_t y = 0;
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

    int32_t padding = DDrawSurfaceWidth <= 800 ? 1 : 2;
    int32_t border = 1;
    int32_t sx = GetRenderScale(x) - padding;
    int32_t sy = GetRenderScale(y) - padding;
    int32_t sw = GetRenderScale(width) * UIBarScale + padding * 2;
    int32_t sh = GetRenderScale(height) * UIBarScale + padding * 2;

    // border
    S_DrawScreenFlatQuad(
        sx - border, sy - border, sw + border, sh + border, rgb_border_dark);
    S_DrawScreenFlatQuad(sx, sy, sw + border, sh + border, rgb_border_light);

    // background
    S_DrawScreenFlatQuad(sx, sy, sw, sh, rgb_bgnd);

    const int32_t blink_interval = 20;
    const int32_t blink_threshold = bar_type == BT_ENEMY_HEALTH ? 0 : 20;
    int32_t blink_time = blink_counter++ % blink_interval;
    int32_t blink =
        percent <= blink_threshold && blink_time > blink_interval / 2;

    if (percent && !blink) {
        width = width * percent / percent_max;

        sx = GetRenderScale(x);
        sy = GetRenderScale(y);
        sw = GetRenderScale(width) * UIBarScale;
        sh = GetRenderScale(height) * UIBarScale;

        if (T1MConfig.enable_smooth_bars) {
            for (int i = 0; i < COLOR_STEPS - 1; i++) {
                RGB888 c1 = ColorBarMap[bar_color][i];
                RGB888 c2 = ColorBarMap[bar_color][i + 1];
                int32_t lsy = sy + i * sh / (COLOR_STEPS - 1);
                int32_t lsh = sy + (i + 1) * sh / (COLOR_STEPS - 1) - lsy;
                S_DrawScreenGradientQuad(sx, lsy, sw, lsh, c1, c1, c2, c2);
            }
        } else {
            for (int i = 0; i < COLOR_STEPS; i++) {
                RGB888 color = ColorBarMap[bar_color][i];
                int32_t lsy = sy + i * sh / COLOR_STEPS;
                int32_t lsh = sy + (i + 1) * sh / COLOR_STEPS - lsy;
                S_DrawScreenFlatQuad(sx, lsy, sw, lsh, color);
            }
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

int32_t S_DumpScreen()
{
    HWR_DumpScreen();
    WinSpinMessageLoop();
    FPSCounter++;
    return ClockSyncTicks(TICKS_PER_FRAME);
}

void S_ClearScreen()
{
    HWR_ClearSurfaceDepth();
}

void S_OutputPolyList()
{
    HWR_OutputPolyList();
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

        PHD_VECTOR ls = { 0, 0, 0 };
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

void S_SetupBelowWater(int32_t underwater)
{
    PhdWet = underwater;
    IsWaterEffect = 1;
    IsWibbleEffect = underwater == 0;
    IsShadeEffect = 1;
}

void S_SetupAboveWater(int32_t underwater)
{
    IsWaterEffect = 0;
    IsWibbleEffect = underwater;
    IsShadeEffect = underwater;
}

void S_AnimateTextures(int32_t ticks)
{
    WibbleOffset = (WibbleOffset + ticks) % WIBBLE_SIZE;

    static int32_t tick_comp = 0;
    tick_comp += ticks;

    while (tick_comp > TICKS_PER_FRAME * 5) {
        int16_t *ptr = AnimTextureRanges;
        int16_t i = *ptr++;
        while (i > 0) {
            int16_t j = *ptr++;
            PHD_TEXTURE temp = PhdTextureInfo[*ptr];
            while (j > 0) {
                PhdTextureInfo[ptr[0]] = PhdTextureInfo[ptr[1]];
                j--;
                ptr++;
            }
            PhdTextureInfo[*ptr] = temp;
            i--;
            ptr++;
        }
        tick_comp -= TICKS_PER_FRAME * 5;
    }
}

static int DecompPCX(const char *pcx, size_t pcx_size, char *pic, RGB888 *pal)
{
    PCX_HEADER *header = (PCX_HEADER *)pcx;
    int32_t width = header->x_max - header->x_min + 1;
    int32_t height = header->y_max - header->y_min + 1;

    if (header->manufacturer != 10 || header->version < 5 || header->bpp != 8
        || header->rle != 1 || header->planes != 1 || width * height == 0) {
        return 0;
    }

    const int32_t stride = width + width % 2;
    const char *src = pcx + sizeof(PCX_HEADER);
    char *dst = pic;
    int32_t x = 0;
    int32_t y = 0;

    while (y < height) {
        if ((*src & 0xC0) == 0xC0) {
            uint8_t n = (*src++) & 0x3F;
            uint8_t c = *src++;
            if (n > 0) {
                if (x < width) {
                    CLAMPG(n, width - x);
                    for (int i = 0; i < n; i++) {
                        *dst++ = c;
                    }
                }
                x += n;
            }
        } else {
            *dst++ = *src++;
            x++;
        }
        if (x >= stride) {
            x = 0;
            y++;
        }
    }

    if (pal != NULL) {
        src = pcx + pcx_size - sizeof(RGB888) * 256;
        for (int i = 0; i < 256; i++) {
            pal[i].r = ((*src++) >> 2) & 0x3F;
            pal[i].g = ((*src++) >> 2) & 0x3F;
            pal[i].b = ((*src++) >> 2) & 0x3F;
        }
    }

    return 1;
}

void S_DisplayPicture(const char *file_stem)
{
    char file_name[128];
    strcpy(file_name, file_stem);
    strcat(file_name, ".pcx");
    const char *file_path = GetFullPath(file_name);

    char *file_data = NULL;
    size_t file_size = 0;
    FileLoad(file_path, &file_data, &file_size);

    if (!DecompPCX(file_data, file_size, (char *)ScrPtr, GamePalette)) {
        LOG_ERROR("failed to decompress PCX %s", file_path);
    }

    free(file_data);

    HWR_DownloadPicture();
}

void S_DrawLightningSegment(
    int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2,
    int32_t width)
{
    if (z1 >= PhdNearZ && z1 <= PhdFarZ && z2 >= PhdNearZ && z2 <= PhdFarZ) {
        x1 = PhdCenterX + x1 / (z1 / PhdPersp);
        y1 = PhdCenterY + y1 / (z1 / PhdPersp);
        x2 = PhdCenterX + x2 / (z2 / PhdPersp);
        y2 = PhdCenterY + y2 / (z2 / PhdPersp);
        int32_t thickness1 = (width << W2V_SHIFT) / (z1 / PhdPersp);
        int32_t thickness2 = (width << W2V_SHIFT) / (z2 / PhdPersp);
        HWR_DrawLightningSegment(
            x1, y1, z1, thickness1, x2, y2, z2, thickness2);
    }
}

void S_PrintShadow(int16_t size, int16_t *bptr, ITEM_INFO *item)
{
    int i;

    ShadowInfo.vertex_count = T1MConfig.enable_round_shadow ? 32 : 8;

    int32_t x0 = bptr[FRAME_BOUND_MIN_X];
    int32_t x1 = bptr[FRAME_BOUND_MAX_X];
    int32_t z0 = bptr[FRAME_BOUND_MIN_Z];
    int32_t z1 = bptr[FRAME_BOUND_MAX_Z];

    int32_t x_mid = (x0 + x1) / 2;
    int32_t z_mid = (z0 + z1) / 2;

    int32_t x_add = (x1 - x0) * size / 1024;
    int32_t z_zdd = (z1 - z0) * size / 1024;

    for (i = 0; i < ShadowInfo.vertex_count; i++) {
        int32_t angle = (PHD_180 + i * PHD_360) / ShadowInfo.vertex_count;
        ShadowInfo.vertex[i].x =
            z_mid + (x_add * 2) * phd_sin(angle) / (PHD_ONE / 4);
        ShadowInfo.vertex[i].z =
            z_mid + (x_add * 2) * phd_cos(angle) / (PHD_ONE / 4);
        ShadowInfo.vertex[i].y = 0;
    }

    phd_PushMatrix();
    phd_TranslateAbs(item->pos.x, item->floor, item->pos.z);
    phd_RotY(item->pos.y_rot);
    if (calc_object_vertices(&ShadowInfo.poly_count)) {
        int16_t clip_and = 1;
        int16_t clip_positive = 1;
        int16_t clip_or = 0;
        for (i = 0; i < ShadowInfo.vertex_count; i++) {
            clip_and &= PhdVBuf[i].clip;
            clip_positive &= PhdVBuf[i].clip >= 0;
            clip_or |= PhdVBuf[i].clip;
        }
        PHD_VBUF *vn1 = &PhdVBuf[0];
        PHD_VBUF *vn2 = &PhdVBuf[1];
        PHD_VBUF *vn3 = &PhdVBuf[2];

        if (!clip_and && clip_positive && VISIBLE(vn1, vn2, vn3)) {
            HWR_PrintShadow(
                &PhdVBuf[0], clip_or ? 1 : 0, ShadowInfo.vertex_count);
        }
    }

    phd_PopMatrix();
}

int S_GetObjectBounds(int16_t *bptr)
{
    if (PhdMatrixPtr->_23 >= PhdFarZ) {
        return 0;
    }

    int32_t x_min = bptr[0];
    int32_t x_max = bptr[1];
    int32_t y_min = bptr[2];
    int32_t y_max = bptr[3];
    int32_t z_min = bptr[4];
    int32_t z_max = bptr[5];

    PHD_VECTOR vtx[8];
    vtx[0].x = x_min;
    vtx[0].y = y_min;
    vtx[0].z = z_min;
    vtx[1].x = x_max;
    vtx[1].y = y_min;
    vtx[1].z = z_min;
    vtx[2].x = x_max;
    vtx[2].y = y_max;
    vtx[2].z = z_min;
    vtx[3].x = x_min;
    vtx[3].y = y_max;
    vtx[3].z = z_min;
    vtx[4].x = x_min;
    vtx[4].y = y_min;
    vtx[4].z = z_max;
    vtx[5].x = x_max;
    vtx[5].y = y_min;
    vtx[5].z = z_max;
    vtx[6].x = x_max;
    vtx[6].y = y_max;
    vtx[6].z = z_max;
    vtx[7].x = x_min;
    vtx[7].y = y_max;
    vtx[7].z = z_max;

    int num_z = 0;
    x_min = 0x3FFFFFFF;
    y_min = 0x3FFFFFFF;
    x_max = -0x3FFFFFFF;
    y_max = -0x3FFFFFFF;

    for (int i = 0; i < 8; i++) {
        int32_t zv = PhdMatrixPtr->_20 * vtx[i].x + PhdMatrixPtr->_21 * vtx[i].y
            + PhdMatrixPtr->_22 * vtx[i].z + PhdMatrixPtr->_23;

        if (zv > PhdNearZ && zv < PhdFarZ) {
            ++num_z;
            int32_t zp = zv / PhdPersp;
            int32_t xv =
                (PhdMatrixPtr->_00 * vtx[i].x + PhdMatrixPtr->_01 * vtx[i].y
                 + PhdMatrixPtr->_02 * vtx[i].z + PhdMatrixPtr->_03)
                / zp;
            int32_t yv =
                (PhdMatrixPtr->_10 * vtx[i].x + PhdMatrixPtr->_11 * vtx[i].y
                 + PhdMatrixPtr->_12 * vtx[i].z + PhdMatrixPtr->_13)
                / zp;

            if (x_min > xv) {
                x_min = xv;
            } else if (x_max < xv) {
                x_max = xv;
            }

            if (y_min > yv) {
                y_min = yv;
            } else if (y_max < yv) {
                y_max = yv;
            }
        }
    }

    x_min += PhdCenterX;
    x_max += PhdCenterX;
    y_min += PhdCenterY;
    y_max += PhdCenterY;

    if (!num_z || x_min > PhdRight || y_min > PhdBottom || x_max < PhdLeft
        || y_max < PhdTop) {
        return 0; // out of screen
    }

    if (num_z < 8 || x_min < 0 || y_min < 0 || x_max > PhdWinMaxX
        || y_max > PhdWinMaxY) {
        return -1; // clipped
    }

    return 1; // fully on screen
}

void T1MInjectSpecificOutput()
{
    INJECT(0x0042FC60, S_InitialisePolyList);
    INJECT(0x0042FC70, S_DumpScreen);
    INJECT(0x0042FCC0, S_ClearScreen);
    INJECT(0x0042FCE0, S_InitialiseScreen);
    INJECT(0x0042FD10, S_OutputPolyList);
    INJECT(0x0042FD30, S_GetObjectBounds);
    INJECT(0x0042FFA0, S_PrintShadow);
    INJECT(0x00430100, S_CalculateLight);
    INJECT(0x00430290, S_CalculateStaticLight);
    INJECT(0x004302D0, S_DrawHealthBar);
    INJECT(0x00430450, S_DrawAirBar);
    INJECT(0x004305E0, S_SetupBelowWater);
    INJECT(0x00430640, S_SetupAboveWater);
    INJECT(0x00430740, S_DrawLightningSegment);
    INJECT(0x00430CE0, S_DisplayPicture);
}
