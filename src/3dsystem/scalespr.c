#include "3dsystem/scalespr.h"

#include "3dsystem/3d_gen.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/s_hwr.h"

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
