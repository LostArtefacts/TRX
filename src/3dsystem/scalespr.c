#include "3dsystem/scalespr.h"

#include "global/const.h"
#include "global/vars.h"
#include "global/types.h"
#include "specific/hwr.h"

void S_DrawSprite(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade)
{
    x -= W2VMatrix._03;
    y -= W2VMatrix._13;
    z -= W2VMatrix._23;

    if (x < -PhdViewDist || x > PhdViewDist) {
        return;
    }

    if (y < -PhdViewDist || y > PhdViewDist) {
        return;
    }

    if (z < -PhdViewDist || z > PhdViewDist) {
        return;
    }

    int32_t zv = W2VMatrix._20 * x + W2VMatrix._21 * y + W2VMatrix._22 * z;
    if (zv < PhdNearZ || zv > PhdFarZ) {
        return;
    }

    int32_t xv = W2VMatrix._00 * x + W2VMatrix._01 * y + W2VMatrix._02 * z;
    int32_t yv = W2VMatrix._10 * x + W2VMatrix._11 * y + W2VMatrix._12 * z;
    int32_t zp = zv / PhdPersp;

    PHD_SPRITE *sprite = &PhdSpriteInfo[sprnum];
    int32_t x1 = PhdCenterX + (xv + (sprite->x1 << W2V_SHIFT)) / zp;
    int32_t y1 = PhdCenterY + (yv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t x2 = PhdCenterX + (xv + (sprite->x2 << W2V_SHIFT)) / zp;
    int32_t y2 = PhdCenterY + (yv + (sprite->y2 << W2V_SHIFT)) / zp;
    if (x2 >= 0 && y2 >= 0 && x1 < PhdWinWidth && y1 < PhdWinHeight) {
        int32_t depth = zv >> W2V_SHIFT;
        if (depth > DEPTH_Q_START) {
            shade += depth - DEPTH_Q_START;
            if (shade > 0x1FFF) {
                return;
            }
        }
        HWR_DrawSprite(x1, y1, x2, y2, zv, sprnum, shade);
    }
}

void S_DrawSpriteRel(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade)
{
    int32_t zv = PhdMatrixPtr->_20 * x + PhdMatrixPtr->_21 * y
        + PhdMatrixPtr->_22 * z + PhdMatrixPtr->_23;
    if (zv < PhdNearZ || zv > PhdFarZ) {
        return;
    }

    int32_t xv = PhdMatrixPtr->_00 * x + PhdMatrixPtr->_01 * y
        + PhdMatrixPtr->_02 * z + PhdMatrixPtr->_03;
    int32_t yv = PhdMatrixPtr->_10 * x + PhdMatrixPtr->_11 * y
        + PhdMatrixPtr->_12 * z + PhdMatrixPtr->_13;
    int32_t zp = zv / PhdPersp;

    PHD_SPRITE *sprite = &PhdSpriteInfo[sprnum];
    int32_t x1 = PhdCenterX + (xv + (sprite->x1 << W2V_SHIFT)) / zp;
    int32_t y1 = PhdCenterY + (yv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t x2 = PhdCenterX + (xv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t y2 = PhdCenterY + (yv + (sprite->y2 << W2V_SHIFT)) / zp;
    if (x2 >= 0 && y2 >= 0 && x1 < PhdWinWidth && y1 < PhdWinHeight) {
        int32_t depth = (zv >> W2V_SHIFT);
        if (depth > DEPTH_Q_START) {
            shade += depth - DEPTH_Q_START;
            if (shade > 0x1FFF) {
                return;
            }
        }
        HWR_DrawSprite(x1, y1, x2, y2, zv, sprnum, shade);
    }
}

void S_DrawUISprite(
    int32_t x, int32_t y, int32_t scale, int16_t sprnum, int16_t shade)
{
    PHD_SPRITE *sprite = &PhdSpriteInfo[sprnum];
    int32_t x1 = x + (scale * sprite->x1 >> 16);
    int32_t x2 = x + (scale * sprite->x2 >> 16);
    int32_t y1 = y + (scale * sprite->y1 >> 16);
    int32_t y2 = y + (scale * sprite->y2 >> 16);
    if (x2 >= 0 && y2 >= 0 && x1 < PhdWinWidth && y1 < PhdWinHeight) {
        HWR_DrawSprite(x1, y1, x2, y2, 200, sprnum, shade);
    }
}

const int16_t *S_DrawRoomSprites(const int16_t *obj_ptr, int32_t vertex_count)
{
    for (int i = 0; i < vertex_count; i++) {
        int16_t vbuf_num = obj_ptr[0];
        int16_t sprnum = obj_ptr[1];
        obj_ptr += 2;

        PHD_VBUF *vbuf = &PhdVBuf[vbuf_num];
        if (vbuf->clip < 0) {
            continue;
        }

        int32_t zv = vbuf->zv;
        PHD_SPRITE *sprite = &PhdSpriteInfo[sprnum];
        int32_t zp = (zv / PhdPersp);
        int32_t x1 = PhdCenterX + (vbuf->xv + (sprite->x1 << W2V_SHIFT)) / zp;
        int32_t y1 = PhdCenterY + (vbuf->yv + (sprite->y1 << W2V_SHIFT)) / zp;
        int32_t x2 = PhdCenterX + (vbuf->xv + (sprite->x2 << W2V_SHIFT)) / zp;
        int32_t y2 = PhdCenterY + (vbuf->yv + (sprite->y2 << W2V_SHIFT)) / zp;
        if (x2 >= PhdLeft && y2 >= PhdTop && x1 < PhdRight && y1 < PhdBottom) {
            HWR_DrawSprite(x1, y1, x2, y2, zv, sprnum, vbuf->g);
        }
    }

    return obj_ptr;
}

void T1MInject3DSystemScaleSpr()
{
    INJECT(0x00435910, S_DrawSprite);
    INJECT(0x00435B70, S_DrawSpriteRel);
    INJECT(0x00435D80, S_DrawUISprite);
    INJECT(0x00435ED0, S_DrawRoomSprites);
}
