#include "specific/s_output.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "config.h"
#include "filesystem.h"
#include "game/clock.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/picture.h"
#include "game/screen.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "log.h"
#include "memory.h"
#include "specific/s_hwr.h"
#include "specific/s_shell.h"

#include <assert.h>
#include <string.h>

void S_NoFade()
{
    // not implemented in TombATI
}

void S_FadeInInventory(int32_t fade)
{
    if (g_CurrentLevel != g_GameFlow.title_level_num) {
        HWR_CopyFromPicture();
    }
}

void S_FadeOutInventory(int32_t fade)
{
    // not implemented in TombATI
}

void S_CopyBufferToScreen()
{
    HWR_CopyToPicture();
}

SG_COL S_Colour(int32_t red, int32_t green, int32_t blue)
{
    int32_t best_dist = SQUARE(256) * 3;
    SG_COL best_entry = 0;
    for (int i = 0; i < 256; i++) {
        RGB888 *col = &g_GamePalette[i];
        int32_t dr = red - col->r;
        int32_t dg = green - col->g;
        int32_t db = blue - col->b;
        int32_t dist = SQUARE(dr) + SQUARE(dg) + SQUARE(db);
        if (dist < best_dist) {
            best_dist = dist;
            best_entry = i;
        }
    }
    return best_entry;
}

RGB888 S_ColourFromPalette(int8_t idx)
{
    RGB888 ret;
    ret.r = 4 * g_GamePalette[idx].r;
    ret.g = 4 * g_GamePalette[idx].g;
    ret.b = 4 * g_GamePalette[idx].b;
    return ret;
}

void S_DrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 color)
{
    HWR_Draw2DQuad(sx, sy, sx + w, sy + h, color, color, color, color);
}

void S_DrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 tl, RGB888 tr,
    RGB888 bl, RGB888 br)
{
    HWR_Draw2DQuad(sx, sy, sx + w, sy + h, tl, tr, bl, br);
}

void S_DrawScreenLine(int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 col)
{
    HWR_Draw2DLine(sx, sy, sx + w, sy + h, col, col);
}

void S_DrawScreenBox(int32_t sx, int32_t sy, int32_t w, int32_t h)
{
    RGB888 rgb_border_light = S_ColourFromPalette(15);
    RGB888 rgb_border_dark = S_ColourFromPalette(31);
    S_DrawScreenLine(sx - 1, sy - 1, w + 3, 0, rgb_border_light);
    S_DrawScreenLine(sx, sy, w + 1, 0, rgb_border_dark);
    S_DrawScreenLine(w + sx + 1, sy, 0, h + 1, rgb_border_light);
    S_DrawScreenLine(w + sx + 2, sy - 1, 0, h + 3, rgb_border_dark);
    S_DrawScreenLine(w + sx + 1, h + sy + 1, -w - 1, 0, rgb_border_light);
    S_DrawScreenLine(w + sx + 2, h + sy + 2, -w - 3, 0, rgb_border_dark);
    S_DrawScreenLine(sx - 1, h + sy + 2, 0, -3 - h, rgb_border_light);
    S_DrawScreenLine(sx, h + sy + 1, 0, -1 - h, rgb_border_dark);
}

void S_DrawScreenFBox(int32_t sx, int32_t sy, int32_t w, int32_t h)
{
    HWR_DrawTranslucentQuad(sx, sy, sx + w, sy + h);
}

void S_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int16_t sprnum, int16_t shade, uint16_t flags)
{
    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = sx + (scale_h * (sprite->x1 >> 3) / PHD_ONE);
    int32_t x2 = sx + (scale_h * (sprite->x2 >> 3) / PHD_ONE);
    int32_t y1 = sy + (scale_v * (sprite->y1 >> 3) / PHD_ONE);
    int32_t y2 = sy + (scale_v * (sprite->y2 >> 3) / PHD_ONE);
    if (x2 >= 0 && y2 >= 0 && x1 < ViewPort_GetWidth()
        && y1 < ViewPort_GetHeight()) {
        HWR_DrawSprite(x1, y1, x2, y2, 8 * z, sprnum, shade);
    }
}

void S_DrawScreenSprite2d(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int32_t sprnum, int16_t shade, uint16_t flags, int32_t page)
{
    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = sx + (scale_h * sprite->x1 / PHD_ONE);
    int32_t x2 = sx + (scale_h * sprite->x2 / PHD_ONE);
    int32_t y1 = sy + (scale_v * sprite->y1 / PHD_ONE);
    int32_t y2 = sy + (scale_v * sprite->y2 / PHD_ONE);
    if (x2 >= 0 && y2 >= 0 && x1 < ViewPort_GetWidth()
        && y1 < ViewPort_GetHeight()) {
        HWR_DrawSprite(x1, y1, x2, y2, 200, sprnum, 0);
    }
}

void S_FinishInventory()
{
    if (g_InvMode != INV_TITLE_MODE) {
        Screen_ApplyResolution();
    }
    g_ModeLock = false;
}

void S_FadeToBlack()
{
    memset(g_GamePalette, 0, sizeof(g_GamePalette));
    HWR_FadeToPal(20, g_GamePalette);
    HWR_FadeWait();
}

void S_Wait(int32_t nticks)
{
    for (int i = 0; i < nticks; i++) {
        Input_Update();
        if (g_Input.any) {
            break;
        }
        Clock_SyncTicks(1);
    }
    while (g_Input.any) {
        Input_Update();
    }
}

void S_InitialisePolyList()
{
    phd_InitPolyList();
}

int32_t S_DumpScreen()
{
    HWR_DumpScreen();
    S_Shell_SpinMessageLoop();
    g_FPSCounter++;
    return Clock_SyncTicks(TICKS_PER_FRAME);
}

void S_ClearScreen()
{
    HWR_ClearBackBuffer();
}

void S_OutputPolyList()
{
    HWR_OutputPolyList();
}

void S_CalculateLight(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    ROOM_INFO *r = &g_RoomInfo[room_num];

    if (r->num_lights == 0) {
        g_LsAdder = r->ambient;
        g_LsDivider = 0;
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

        g_LsAdder = (ambient + brightest) / 2;
        if (brightest == ambient) {
            g_LsDivider = 0;
        } else {
            g_LsDivider = (1 << (W2V_SHIFT + 12)) / (brightest - g_LsAdder);
        }
        g_LsAdder = 0x1FFF - g_LsAdder;

        PHD_ANGLE angles[2];
        phd_GetVectorAngles(ls.x, ls.y, ls.z, angles);
        phd_RotateLight(angles[1], angles[0]);
    }

    int32_t distance = g_PhdMatrixPtr->_23 >> W2V_SHIFT;
    g_LsAdder += phd_CalculateFogShade(distance);
    CLAMPG(g_LsAdder, 0x1FFF);
}

void S_CalculateStaticLight(int16_t adder)
{
    g_LsAdder = adder - 16 * 256;
    int32_t distance = g_PhdMatrixPtr->_23 >> W2V_SHIFT;
    g_LsAdder += phd_CalculateFogShade(distance);
    CLAMPG(g_LsAdder, 0x1FFF);
}

void S_SetupBelowWater(bool underwater)
{
    g_IsWaterEffect = true;
    g_IsWibbleEffect = !underwater;
    g_IsShadeEffect = true;
}

void S_SetupAboveWater(bool underwater)
{
    g_IsWaterEffect = false;
    g_IsWibbleEffect = underwater;
    g_IsShadeEffect = underwater;
}

void S_AnimateTextures(int32_t ticks)
{
    g_WibbleOffset = (g_WibbleOffset + ticks) % WIBBLE_SIZE;

    static int32_t tick_comp = 0;
    tick_comp += ticks;

    while (tick_comp > TICKS_PER_FRAME * 5) {
        int16_t *ptr = g_AnimTextureRanges;
        int16_t i = *ptr++;
        while (i > 0) {
            int16_t j = *ptr++;
            PHD_TEXTURE temp = g_PhdTextureInfo[*ptr];
            while (j > 0) {
                g_PhdTextureInfo[ptr[0]] = g_PhdTextureInfo[ptr[1]];
                j--;
                ptr++;
            }
            g_PhdTextureInfo[*ptr] = temp;
            i--;
            ptr++;
        }
        tick_comp -= TICKS_PER_FRAME * 5;
    }
}

void S_DisplayPicture(const char *filename)
{
    PICTURE *orig_pic = Picture_CreateFromFile(filename);
    if (orig_pic) {
        PICTURE *scaled_pic = Picture_Create();
        if (scaled_pic) {
            Picture_Scale(
                scaled_pic, orig_pic, ViewPort_GetWidth(),
                ViewPort_GetHeight());
            HWR_DownloadPicture(scaled_pic);
            Picture_Free(scaled_pic);
        }
        Picture_Free(orig_pic);
    }
}

void S_DrawLightningSegment(
    int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2,
    int32_t width)
{
    if (z1 >= phd_GetNearZ() && z1 <= phd_GetFarZ() && z2 >= phd_GetNearZ()
        && z2 <= phd_GetFarZ()) {
        x1 = ViewPort_GetCenterX() + x1 / (z1 / g_PhdPersp);
        y1 = ViewPort_GetCenterY() + y1 / (z1 / g_PhdPersp);
        x2 = ViewPort_GetCenterX() + x2 / (z2 / g_PhdPersp);
        y2 = ViewPort_GetCenterY() + y2 / (z2 / g_PhdPersp);
        int32_t thickness1 = (width << W2V_SHIFT) / (z1 / g_PhdPersp);
        int32_t thickness2 = (width << W2V_SHIFT) / (z2 / g_PhdPersp);
        HWR_DrawLightningSegment(
            x1, y1, z1, thickness1, x2, y2, z2, thickness2);
    }
}

void S_PrintShadow(int16_t size, int16_t *bptr, ITEM_INFO *item)
{
    int i;

    g_ShadowInfo.vertex_count = g_Config.enable_round_shadow ? 32 : 8;

    int32_t x0 = bptr[FRAME_BOUND_MIN_X];
    int32_t x1 = bptr[FRAME_BOUND_MAX_X];
    int32_t z0 = bptr[FRAME_BOUND_MIN_Z];
    int32_t z1 = bptr[FRAME_BOUND_MAX_Z];

    int32_t x_mid = (x0 + x1) / 2;
    int32_t z_mid = (z0 + z1) / 2;

    int32_t x_add = (x1 - x0) * size / 1024;
    int32_t z_add = (z1 - z0) * size / 1024;

    for (i = 0; i < g_ShadowInfo.vertex_count; i++) {
        int32_t angle = (PHD_180 + i * PHD_360) / g_ShadowInfo.vertex_count;
        g_ShadowInfo.vertex[i].x =
            x_mid + (x_add * 2) * phd_sin(angle) / PHD_90;
        g_ShadowInfo.vertex[i].z =
            z_mid + (z_add * 2) * phd_cos(angle) / PHD_90;
        g_ShadowInfo.vertex[i].y = 0;
    }

    phd_PushMatrix();
    phd_TranslateAbs(item->pos.x, item->floor, item->pos.z);
    phd_RotY(item->pos.y_rot);

    if (calc_object_vertices(&g_ShadowInfo.poly_count)) {
        int16_t clip_and = 1;
        int16_t clip_positive = 1;
        int16_t clip_or = 0;
        for (i = 0; i < g_ShadowInfo.vertex_count; i++) {
            clip_and &= g_PhdVBuf[i].clip;
            clip_positive &= g_PhdVBuf[i].clip >= 0;
            clip_or |= g_PhdVBuf[i].clip;
        }
        PHD_VBUF *vn1 = &g_PhdVBuf[0];
        PHD_VBUF *vn2 = &g_PhdVBuf[g_Config.enable_round_shadow ? 4 : 1];
        PHD_VBUF *vn3 = &g_PhdVBuf[g_Config.enable_round_shadow ? 8 : 2];

        bool visible =
            ((int32_t)(((vn3->xs - vn2->xs) * (vn1->ys - vn2->ys)) - ((vn1->xs - vn2->xs) * (vn3->ys - vn2->ys)))
             >= 0);

        if (!clip_and && clip_positive && visible) {
            HWR_PrintShadow(
                &g_PhdVBuf[0], clip_or ? 1 : 0, g_ShadowInfo.vertex_count);
        }
    }

    phd_PopMatrix();
}

int S_GetObjectBounds(int16_t *bptr)
{
    if (g_PhdMatrixPtr->_23 >= phd_GetFarZ()) {
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
        int32_t zv = g_PhdMatrixPtr->_20 * vtx[i].x
            + g_PhdMatrixPtr->_21 * vtx[i].y + g_PhdMatrixPtr->_22 * vtx[i].z
            + g_PhdMatrixPtr->_23;

        if (zv > phd_GetNearZ() && zv < phd_GetFarZ()) {
            ++num_z;
            int32_t zp = zv / g_PhdPersp;
            int32_t xv =
                (g_PhdMatrixPtr->_00 * vtx[i].x + g_PhdMatrixPtr->_01 * vtx[i].y
                 + g_PhdMatrixPtr->_02 * vtx[i].z + g_PhdMatrixPtr->_03)
                / zp;
            int32_t yv =
                (g_PhdMatrixPtr->_10 * vtx[i].x + g_PhdMatrixPtr->_11 * vtx[i].y
                 + g_PhdMatrixPtr->_12 * vtx[i].z + g_PhdMatrixPtr->_13)
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

    x_min += ViewPort_GetCenterX();
    x_max += ViewPort_GetCenterX();
    y_min += ViewPort_GetCenterY();
    y_max += ViewPort_GetCenterY();

    if (!num_z || x_min > g_PhdRight || y_min > g_PhdBottom || x_max < g_PhdLeft
        || y_max < g_PhdTop) {
        return 0; // out of screen
    }

    if (num_z < 8 || x_min < 0 || y_min < 0 || x_max > ViewPort_GetMaxX()
        || y_max > ViewPort_GetMaxY()) {
        return -1; // clipped
    }

    return 1; // fully on screen
}

void S_DrawSprite(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade)
{
    x -= g_W2VMatrix._03;
    y -= g_W2VMatrix._13;
    z -= g_W2VMatrix._23;

    if (x < -phd_GetDrawDistMax() || x > phd_GetDrawDistMax()) {
        return;
    }

    if (y < -phd_GetDrawDistMax() || y > phd_GetDrawDistMax()) {
        return;
    }

    if (z < -phd_GetDrawDistMax() || z > phd_GetDrawDistMax()) {
        return;
    }

    int32_t zv =
        g_W2VMatrix._20 * x + g_W2VMatrix._21 * y + g_W2VMatrix._22 * z;
    if (zv < phd_GetNearZ() || zv > phd_GetFarZ()) {
        return;
    }

    int32_t xv =
        g_W2VMatrix._00 * x + g_W2VMatrix._01 * y + g_W2VMatrix._02 * z;
    int32_t yv =
        g_W2VMatrix._10 * x + g_W2VMatrix._11 * y + g_W2VMatrix._12 * z;
    int32_t zp = zv / g_PhdPersp;

    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = ViewPort_GetCenterX() + (xv + (sprite->x1 << W2V_SHIFT)) / zp;
    int32_t y1 = ViewPort_GetCenterY() + (yv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t x2 = ViewPort_GetCenterX() + (xv + (sprite->x2 << W2V_SHIFT)) / zp;
    int32_t y2 = ViewPort_GetCenterY() + (yv + (sprite->y2 << W2V_SHIFT)) / zp;
    if (x2 >= ViewPort_GetMinX() && y2 >= ViewPort_GetMinY()
        && x1 <= ViewPort_GetMaxX() && y1 <= ViewPort_GetMaxY()) {
        int32_t depth = zv >> W2V_SHIFT;
        shade += phd_CalculateFogShade(depth);
        CLAMPG(shade, 0x1FFF);
        HWR_DrawSprite(x1, y1, x2, y2, zv, sprnum, shade);
    }
}

void S_DrawSpriteRel(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade)
{
    int32_t zv = g_PhdMatrixPtr->_20 * x + g_PhdMatrixPtr->_21 * y
        + g_PhdMatrixPtr->_22 * z + g_PhdMatrixPtr->_23;
    if (zv < phd_GetNearZ() || zv > phd_GetFarZ()) {
        return;
    }

    int32_t xv = g_PhdMatrixPtr->_00 * x + g_PhdMatrixPtr->_01 * y
        + g_PhdMatrixPtr->_02 * z + g_PhdMatrixPtr->_03;
    int32_t yv = g_PhdMatrixPtr->_10 * x + g_PhdMatrixPtr->_11 * y
        + g_PhdMatrixPtr->_12 * z + g_PhdMatrixPtr->_13;
    int32_t zp = zv / g_PhdPersp;

    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = ViewPort_GetCenterX() + (xv + (sprite->x1 << W2V_SHIFT)) / zp;
    int32_t y1 = ViewPort_GetCenterY() + (yv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t x2 = ViewPort_GetCenterX() + (xv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t y2 = ViewPort_GetCenterY() + (yv + (sprite->y2 << W2V_SHIFT)) / zp;
    if (x2 >= ViewPort_GetMinX() && y2 >= ViewPort_GetMinY()
        && x1 <= ViewPort_GetMaxX() && y1 <= ViewPort_GetMaxY()) {
        int32_t depth = zv >> W2V_SHIFT;
        shade += phd_CalculateFogShade(depth);
        CLAMPG(shade, 0x1FFF);
        HWR_DrawSprite(x1, y1, x2, y2, zv, sprnum, shade);
    }
}

void S_DrawUISprite(
    int32_t x, int32_t y, int32_t scale, int16_t sprnum, int16_t shade)
{
    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = x + (scale * sprite->x1 >> 16);
    int32_t x2 = x + (scale * sprite->x2 >> 16);
    int32_t y1 = y + (scale * sprite->y1 >> 16);
    int32_t y2 = y + (scale * sprite->y2 >> 16);
    if (x2 >= ViewPort_GetMinX() && y2 >= ViewPort_GetMinY()
        && x1 <= ViewPort_GetMaxX() && y1 <= ViewPort_GetMaxY()) {
        HWR_DrawSprite(x1, y1, x2, y2, 200, sprnum, shade);
    }
}

const int16_t *S_DrawRoomSprites(const int16_t *obj_ptr, int32_t vertex_count)
{
    for (int i = 0; i < vertex_count; i++) {
        int16_t vbuf_num = obj_ptr[0];
        int16_t sprnum = obj_ptr[1];
        obj_ptr += 2;

        PHD_VBUF *vbuf = &g_PhdVBuf[vbuf_num];
        if (vbuf->clip < 0) {
            continue;
        }

        int32_t zv = vbuf->zv;
        PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
        int32_t zp = (zv / g_PhdPersp);
        int32_t x1 =
            ViewPort_GetCenterX() + (vbuf->xv + (sprite->x1 << W2V_SHIFT)) / zp;
        int32_t y1 =
            ViewPort_GetCenterY() + (vbuf->yv + (sprite->y1 << W2V_SHIFT)) / zp;
        int32_t x2 =
            ViewPort_GetCenterX() + (vbuf->xv + (sprite->x2 << W2V_SHIFT)) / zp;
        int32_t y2 =
            ViewPort_GetCenterY() + (vbuf->yv + (sprite->y2 << W2V_SHIFT)) / zp;
        if (x2 >= g_PhdLeft && y2 >= g_PhdTop && x1 < g_PhdRight
            && y1 < g_PhdBottom) {
            HWR_DrawSprite(x1, y1, x2, y2, zv, sprnum, vbuf->g);
        }
    }

    return obj_ptr;
}
