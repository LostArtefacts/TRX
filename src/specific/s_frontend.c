#include "specific/s_frontend.h"

#include "game/clock.h"
#include "game/input.h"
#include "game/screen.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/s_hwr.h"

#include <dinput.h>

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
        Screen_RestoreResolution();
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
