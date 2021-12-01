#include "3dsystem/3d_gen.h"

#include "3dsystem/phd_math.h"
#include "3dsystem/matrix.h"
#include "config.h"
#include "game/screen.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/vars.h"
#include "specific/s_hwr.h"

#include <math.h>

static PHD_VBUF m_VBuf[1500] = { 0 };
static PHD_VECTOR m_LsVectorView = { 0 };
static int32_t m_DrawDistFade = 0;
static int32_t m_DrawDistMax = 0;

static const int16_t *phd_PutObjectG3(const int16_t *obj_ptr, int32_t number);
static const int16_t *phd_PutObjectG4(const int16_t *obj_ptr, int32_t number);
static const int16_t *phd_PutObjectGT3(const int16_t *obj_ptr, int32_t number);
static const int16_t *phd_PutObjectGT4(const int16_t *obj_ptr, int32_t number);
static const int16_t *phd_PutRoomSprites(
    const int16_t *obj_ptr, int32_t vertex_count);

void phd_LookAt(
    int32_t xsrc, int32_t ysrc, int32_t zsrc, int32_t xtar, int32_t ytar,
    int32_t ztar, int16_t roll)
{
    PHD_ANGLE angles[2];
    phd_GetVectorAngles(xtar - xsrc, ytar - ysrc, ztar - zsrc, angles);

    PHD_3DPOS viewer;
    viewer.x = xsrc;
    viewer.y = ysrc;
    viewer.z = zsrc;
    viewer.x_rot = angles[1];
    viewer.y_rot = angles[0];
    viewer.z_rot = roll;
    phd_GenerateW2V(&viewer);
}

void phd_GetVectorAngles(int32_t x, int32_t y, int32_t z, int16_t *dest)
{
    dest[0] = phd_atan(z, x);

    while ((int16_t)x != x || (int16_t)y != y || (int16_t)z != z) {
        x >>= 2;
        y >>= 2;
        z >>= 2;
    }

    PHD_ANGLE pitch = phd_atan(phd_sqrt(SQUARE(x) + SQUARE(z)), y);
    if ((y > 0 && pitch > 0) || (y < 0 && pitch < 0)) {
        pitch = -pitch;
    }

    dest[1] = pitch;
}

int32_t phd_VisibleZClip(PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3)
{
    double v1x = vn1->xv;
    double v1y = vn1->yv;
    double v1z = vn1->zv;
    double v2x = vn2->xv;
    double v2y = vn2->yv;
    double v2z = vn2->zv;
    double v3x = vn3->xv;
    double v3y = vn3->yv;
    double v3z = vn3->zv;
    double a = v3y * v1x - v1y * v3x;
    double b = v3x * v1z - v1x * v3z;
    double c = v3z * v1y - v1z * v3y;
    return a * v2z + b * v2y + c * v2x < 0.0;
}

void phd_RotateLight(int16_t pitch, int16_t yaw)
{
    int32_t cp = phd_cos(pitch);
    int32_t sp = phd_sin(pitch);
    int32_t cy = phd_cos(yaw);
    int32_t sy = phd_sin(yaw);
    int32_t ls_x = TRIGMULT2(cp, sy);
    int32_t ls_y = -sp;
    int32_t ls_z = TRIGMULT2(cp, cy);
    m_LsVectorView.x = (g_W2VMatrix._00 * ls_x + g_W2VMatrix._01 * ls_y
                        + g_W2VMatrix._02 * ls_z)
        >> W2V_SHIFT;
    m_LsVectorView.y = (g_W2VMatrix._10 * ls_x + g_W2VMatrix._11 * ls_y
                        + g_W2VMatrix._12 * ls_z)
        >> W2V_SHIFT;
    m_LsVectorView.z = (g_W2VMatrix._20 * ls_x + g_W2VMatrix._21 * ls_y
                        + g_W2VMatrix._22 * ls_z)
        >> W2V_SHIFT;
}

void phd_AlterFOV(PHD_ANGLE fov)
{
    // In places that use GAME_FOV, it can be safely changed to user's choice.
    // But for cinematics, the FOV value chosen by devs needs to stay
    // unchanged, otherwise the game renders the low camera in the Lost Valley
    // cutscene wrong.
    if (g_Config.fov_vertical) {
        double aspect_ratio =
            Screen_GetResWidth() / (double)Screen_GetResHeight();
        double fov_rad_h = fov * M_PI / 32760;
        double fov_rad_v = 2 * atan(aspect_ratio * tan(fov_rad_h / 2));
        fov = round((fov_rad_v / M_PI) * 32760);
    }

    int16_t c = phd_cos(fov / 2);
    int16_t s = phd_sin(fov / 2);
    g_PhdPersp = ((Screen_GetResWidth() / 2) * c) / s;
}

const int16_t *calc_object_vertices(const int16_t *obj_ptr)
{
    int16_t total_clip = -1;

    obj_ptr++;
    int vertex_count = *obj_ptr++;
    for (int i = 0; i < vertex_count; i++) {
        int32_t xv = g_PhdMatrixPtr->_00 * obj_ptr[0]
            + g_PhdMatrixPtr->_01 * obj_ptr[1]
            + g_PhdMatrixPtr->_02 * obj_ptr[2] + g_PhdMatrixPtr->_03;
        int32_t yv = g_PhdMatrixPtr->_10 * obj_ptr[0]
            + g_PhdMatrixPtr->_11 * obj_ptr[1]
            + g_PhdMatrixPtr->_12 * obj_ptr[2] + g_PhdMatrixPtr->_13;
        int32_t zv = g_PhdMatrixPtr->_20 * obj_ptr[0]
            + g_PhdMatrixPtr->_21 * obj_ptr[1]
            + g_PhdMatrixPtr->_22 * obj_ptr[2] + g_PhdMatrixPtr->_23;
        m_VBuf[i].xv = xv;
        m_VBuf[i].yv = yv;
        m_VBuf[i].zv = zv;

        int32_t clip_flags;
        if (zv < phd_GetNearZ()) {
            clip_flags = -32768;
        } else {
            clip_flags = 0;

            int32_t xs = ViewPort_GetCenterX() + xv / (zv / g_PhdPersp);
            int32_t ys = ViewPort_GetCenterY() + yv / (zv / g_PhdPersp);

            if (xs < g_PhdLeft) {
                if (xs < -32760) {
                    xs = -32760;
                }
                clip_flags |= 1;
            } else if (xs > g_PhdRight) {
                if (xs > 32760) {
                    xs = 32760;
                }
                clip_flags |= 2;
            }

            if (ys < g_PhdTop) {
                if (ys < -32760) {
                    ys = -32760;
                }
                clip_flags |= 4;
            } else if (ys > g_PhdBottom) {
                if (ys > 32760) {
                    ys = 32760;
                }
                clip_flags |= 8;
            }

            m_VBuf[i].xs = xs;
            m_VBuf[i].ys = ys;
        }

        m_VBuf[i].clip = clip_flags;
        total_clip &= clip_flags;
        obj_ptr += 3;
    }

    return total_clip == 0 ? obj_ptr : NULL;
}

const int16_t *calc_vertice_light(const int16_t *obj_ptr)
{
    int32_t vertex_count = *obj_ptr++;
    if (vertex_count > 0) {
        if (g_LsDivider) {
            int32_t xv = (g_PhdMatrixPtr->_00 * m_LsVectorView.x
                          + g_PhdMatrixPtr->_10 * m_LsVectorView.y
                          + g_PhdMatrixPtr->_20 * m_LsVectorView.z)
                / g_LsDivider;
            int32_t yv = (g_PhdMatrixPtr->_01 * m_LsVectorView.x
                          + g_PhdMatrixPtr->_11 * m_LsVectorView.y
                          + g_PhdMatrixPtr->_21 * m_LsVectorView.z)
                / g_LsDivider;
            int32_t zv = (g_PhdMatrixPtr->_02 * m_LsVectorView.x
                          + g_PhdMatrixPtr->_12 * m_LsVectorView.y
                          + g_PhdMatrixPtr->_22 * m_LsVectorView.z)
                / g_LsDivider;
            for (int i = 0; i < vertex_count; i++) {
                int16_t shade = g_LsAdder
                    + ((obj_ptr[0] * xv + obj_ptr[1] * yv + obj_ptr[2] * zv)
                       >> 16);
                CLAMP(shade, 0, 0x1FFF);
                m_VBuf[i].g = shade;
                obj_ptr += 3;
            }
            return obj_ptr;
        } else {
            int16_t shade = g_LsAdder;
            CLAMP(shade, 0, 0x1FFF);
            for (int i = 0; i < vertex_count; i++) {
                m_VBuf[i].g = shade;
            }
            obj_ptr += 3 * vertex_count;
        }
    } else {
        for (int i = 0; i < -vertex_count; i++) {
            int16_t shade = g_LsAdder + *obj_ptr++;
            CLAMP(shade, 0, 0x1FFF);
            m_VBuf[i].g = shade;
        }
    }
    return obj_ptr;
}

const int16_t *calc_roomvert(const int16_t *obj_ptr)
{
    int32_t vertex_count = *obj_ptr++;

    for (int i = 0; i < vertex_count; i++) {
        int32_t xv = g_PhdMatrixPtr->_00 * obj_ptr[0]
            + g_PhdMatrixPtr->_01 * obj_ptr[1]
            + g_PhdMatrixPtr->_02 * obj_ptr[2] + g_PhdMatrixPtr->_03;
        int32_t yv = g_PhdMatrixPtr->_10 * obj_ptr[0]
            + g_PhdMatrixPtr->_11 * obj_ptr[1]
            + g_PhdMatrixPtr->_12 * obj_ptr[2] + g_PhdMatrixPtr->_13;
        int32_t zv = g_PhdMatrixPtr->_20 * obj_ptr[0]
            + g_PhdMatrixPtr->_21 * obj_ptr[1]
            + g_PhdMatrixPtr->_22 * obj_ptr[2] + g_PhdMatrixPtr->_23;
        m_VBuf[i].xv = xv;
        m_VBuf[i].yv = yv;
        m_VBuf[i].zv = zv;
        m_VBuf[i].g = obj_ptr[3];

        if (zv < phd_GetNearZ()) {
            m_VBuf[i].clip = 0x8000;
        } else {
            int16_t clip_flags = 0;
            int32_t depth = zv >> W2V_SHIFT;
            if (depth > phd_GetDrawDistMax()) {
                m_VBuf[i].g = 0x1FFF;
                clip_flags |= 16;
            } else if (depth) {
                m_VBuf[i].g += phd_CalculateFogShade(depth);
                if (!g_IsWaterEffect) {
                    CLAMPG(m_VBuf[i].g, 0x1FFF);
                }
            }

            int32_t xs = ViewPort_GetCenterX() + xv / (zv / g_PhdPersp);
            int32_t ys = ViewPort_GetCenterY() + yv / (zv / g_PhdPersp);
            if (g_IsWibbleEffect) {
                xs += g_WibbleTable[(ys + g_WibbleOffset) & 0x1F];
                ys += g_WibbleTable[(xs + g_WibbleOffset) & 0x1F];
            }

            if (xs < g_PhdLeft) {
                if (xs < -32760) {
                    xs = -32760;
                }
                clip_flags |= 1;
            } else if (xs > g_PhdRight) {
                if (xs > 32760) {
                    xs = 32760;
                }
                clip_flags |= 2;
            }

            if (ys < g_PhdTop) {
                if (ys < -32760) {
                    ys = -32760;
                }
                clip_flags |= 4;
            } else if (ys > g_PhdBottom) {
                if (ys > 32760) {
                    ys = 32760;
                }
                clip_flags |= 8;
            }

            if (g_IsWaterEffect) {
                m_VBuf[i].g += g_ShadeTable[(
                    ((uint8_t)g_WibbleOffset
                     + (uint8_t)g_RandTable[(vertex_count - i) % WIBBLE_SIZE])
                    % WIBBLE_SIZE)];
                CLAMP(m_VBuf[i].g, 0, 0x1FFF);
            }

            m_VBuf[i].xs = xs;
            m_VBuf[i].ys = ys;
            m_VBuf[i].clip = clip_flags;
        }
        obj_ptr += 4;
    }

    return obj_ptr;
}

void phd_PutPolygons(const int16_t *obj_ptr, int clip)
{
    obj_ptr += 4;
    obj_ptr = calc_object_vertices(obj_ptr);
    if (obj_ptr) {
        obj_ptr = calc_vertice_light(obj_ptr);
        obj_ptr = phd_PutObjectGT4(obj_ptr + 1, *obj_ptr);
        obj_ptr = phd_PutObjectGT3(obj_ptr + 1, *obj_ptr);
        obj_ptr = phd_PutObjectG4(obj_ptr + 1, *obj_ptr);
        obj_ptr = phd_PutObjectG3(obj_ptr + 1, *obj_ptr);
    }
}

void phd_PutPolygons_I(const int16_t *obj_ptr, int32_t clip)
{
    phd_PushMatrix();
    InterpolateMatrix();
    phd_PutPolygons(obj_ptr, clip);
    phd_PopMatrix();
}

void phd_PutRoom(const int16_t *obj_ptr)
{
    obj_ptr = calc_roomvert(obj_ptr);
    obj_ptr = phd_PutObjectGT4(obj_ptr + 1, *obj_ptr);
    obj_ptr = phd_PutObjectGT3(obj_ptr + 1, *obj_ptr);
    obj_ptr = phd_PutRoomSprites(obj_ptr + 1, *obj_ptr);
}

int32_t phd_GetDrawDistMin()
{
    return 127;
}

int32_t phd_GetDrawDistFade()
{
    return m_DrawDistFade;
}

int32_t phd_GetDrawDistMax()
{
    return m_DrawDistMax;
}

void phd_SetDrawDistFade(int32_t dist)
{
    m_DrawDistFade = dist;
}

void phd_SetDrawDistMax(int32_t dist)
{
    m_DrawDistMax = dist;
}

int32_t phd_GetNearZ()
{
    return phd_GetDrawDistMin() << W2V_SHIFT;
}

int32_t phd_GetFarZ()
{
    return phd_GetDrawDistMax() << W2V_SHIFT;
}

int32_t phd_CalculateFogShade(int32_t depth)
{
    int32_t fog_begin = phd_GetDrawDistFade();
    int32_t fog_end = phd_GetDrawDistMax();

    if (depth < fog_begin) {
        return 0;
    }
    if (depth >= fog_end) {
        return 0x1FFF;
    }

    return (depth - fog_begin) * 0x1FFF / (fog_end - fog_begin);
}

static const int16_t *phd_PutObjectG3(const int16_t *obj_ptr, int32_t number)
{
    int32_t i;
    PHD_VBUF *vns[3];
    int32_t color;

    HWR_DisableTextureMode();

    for (i = 0; i < number; i++) {
        vns[0] = &m_VBuf[*obj_ptr++];
        vns[1] = &m_VBuf[*obj_ptr++];
        vns[2] = &m_VBuf[*obj_ptr++];
        color = *obj_ptr++;

        HWR_DrawFlatTriangle(vns[0], vns[1], vns[2], color);
    }

    return obj_ptr;
}

static const int16_t *phd_PutObjectG4(const int16_t *obj_ptr, int32_t number)
{
    int32_t i;
    PHD_VBUF *vns[4];
    int32_t color;

    HWR_DisableTextureMode();

    for (i = 0; i < number; i++) {
        vns[0] = &m_VBuf[*obj_ptr++];
        vns[1] = &m_VBuf[*obj_ptr++];
        vns[2] = &m_VBuf[*obj_ptr++];
        vns[3] = &m_VBuf[*obj_ptr++];
        color = *obj_ptr++;

        HWR_DrawFlatTriangle(vns[0], vns[1], vns[2], color);
        HWR_DrawFlatTriangle(vns[2], vns[3], vns[0], color);
    }

    return obj_ptr;
}

static const int16_t *phd_PutObjectGT3(const int16_t *obj_ptr, int32_t number)
{
    int32_t i;
    PHD_VBUF *vns[3];
    PHD_TEXTURE *tex;

    HWR_EnableTextureMode();

    for (i = 0; i < number; i++) {
        vns[0] = &m_VBuf[*obj_ptr++];
        vns[1] = &m_VBuf[*obj_ptr++];
        vns[2] = &m_VBuf[*obj_ptr++];
        tex = &g_PhdTextureInfo[*obj_ptr++];

        HWR_DrawTexturedTriangle(
            vns[0], vns[1], vns[2], tex->tpage, &tex->uv[0], &tex->uv[1],
            &tex->uv[2], tex->drawtype);
    }

    return obj_ptr;
}

static const int16_t *phd_PutObjectGT4(const int16_t *obj_ptr, int32_t number)
{
    int32_t i;
    PHD_VBUF *vns[4];
    PHD_TEXTURE *tex;

    HWR_EnableTextureMode();

    for (i = 0; i < number; i++) {
        vns[0] = &m_VBuf[*obj_ptr++];
        vns[1] = &m_VBuf[*obj_ptr++];
        vns[2] = &m_VBuf[*obj_ptr++];
        vns[3] = &m_VBuf[*obj_ptr++];
        tex = &g_PhdTextureInfo[*obj_ptr++];

        HWR_DrawTexturedQuad(
            vns[0], vns[1], vns[2], vns[3], tex->tpage, &tex->uv[0],
            &tex->uv[1], &tex->uv[2], &tex->uv[3], tex->drawtype);
    }

    return obj_ptr;
}

static const int16_t *phd_PutRoomSprites(
    const int16_t *obj_ptr, int32_t vertex_count)
{
    for (int i = 0; i < vertex_count; i++) {
        int16_t vbuf_num = obj_ptr[0];
        int16_t sprnum = obj_ptr[1];
        obj_ptr += 2;

        PHD_VBUF *vbuf = &m_VBuf[vbuf_num];
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

void phd_PutShadow(int16_t size, int16_t *bptr, ITEM_INFO *item)
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
            clip_and &= m_VBuf[i].clip;
            clip_positive &= m_VBuf[i].clip >= 0;
            clip_or |= m_VBuf[i].clip;
        }
        PHD_VBUF *vn1 = &m_VBuf[0];
        PHD_VBUF *vn2 = &m_VBuf[g_Config.enable_round_shadow ? 4 : 1];
        PHD_VBUF *vn3 = &m_VBuf[g_Config.enable_round_shadow ? 8 : 2];

        bool visible =
            ((int32_t)(((vn3->xs - vn2->xs) * (vn1->ys - vn2->ys)) - ((vn1->xs - vn2->xs) * (vn3->ys - vn2->ys)))
             >= 0);

        if (!clip_and && clip_positive && visible) {
            HWR_PrintShadow(
                &m_VBuf[0], clip_or ? 1 : 0, g_ShadowInfo.vertex_count);
        }
    }

    phd_PopMatrix();
}
