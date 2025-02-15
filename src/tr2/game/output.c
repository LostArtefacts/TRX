#include "game/output.h"

#include "game/clock.h"
#include "game/inventory_ring.h"
#include "game/random.h"
#include "game/render/common.h"
#include "game/render/priv.h"
#include "game/scaler.h"
#include "game/shell.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/log.h>
#include <libtrx/utils.h>

typedef enum {
    COLOR_BLACK = 0,
    COLOR_GRAY = 1,
    COLOR_WHITE = 2,
    COLOR_RED = 3,
    COLOR_ORANGE = 4,
    COLOR_YELLOW = 5,
    COLOR_DARK_GREEN = 12,
    COLOR_GREEN = 13,
    COLOR_CYAN = 14,
    COLOR_BLUE = 15,
    COLOR_MAGENTA = 16,
    COLOR_NUMBER_OF = 17,
} COLOR_NAME;

typedef struct {
    RGB_888 rgb;
    uint8_t palette_index;
} NAMED_COLOR;

static NAMED_COLOR m_NamedColors[COLOR_NUMBER_OF] = {
    // clang-format off
    [COLOR_BLACK]      = {.rgb = {.r = 0x00, .g = 0x00, .b = 0x00}},
    [COLOR_GRAY]       = {.rgb = {.r = 0x40, .g = 0x40, .b = 0x40}},
    [COLOR_WHITE]      = {.rgb = {.r = 0xFF, .g = 0xFF, .b = 0xFF}},
    [COLOR_RED]        = {.rgb = {.r = 0xFF, .g = 0x00, .b = 0x00}},
    [COLOR_ORANGE]     = {.rgb = {.r = 0xFF, .g = 0x80, .b = 0x00}},
    [COLOR_YELLOW]     = {.rgb = {.r = 0xFF, .g = 0xFF, .b = 0x00}},
    [COLOR_DARK_GREEN] = {.rgb = {.r = 0x00, .g = 0x80, .b = 0x00}},
    [COLOR_GREEN]      = {.rgb = {.r = 0x00, .g = 0xFF, .b = 0x00}},
    [COLOR_CYAN]       = {.rgb = {.r = 0x00, .g = 0xFF, .b = 0xFF}},
    [COLOR_BLUE]       = {.rgb = {.r = 0x00, .g = 0x00, .b = 0xFF}},
    [COLOR_MAGENTA]    = {.rgb = {.r = 0xFF, .g = 0x00, .b = 0xFF}},
    // clang-format on
};

static int32_t m_TickComp = 0;
static int32_t m_RoomLightShades[RLM_NUMBER_OF] = {};
static ROOM_LIGHT_TABLE m_RoomLightTables[WIBBLE_SIZE] = {};
static float m_WibbleTable[32];
static int16_t m_ShadesTable[32];
static int32_t m_RandomTable[32];
static BACKGROUND_TYPE m_BackgroundType = BK_TRANSPARENT;

static void M_CalcRoomVertices(const ROOM_MESH *mesh, int32_t far_clip);
static void M_CalcRoomVerticesWibble(const ROOM_MESH *mesh);
static void M_DrawRoomSprites(const ROOM_MESH *mesh);

static void M_InsertBar(
    int32_t l, int32_t t, int32_t w, int32_t h, int32_t percent,
    COLOR_NAME bar_color_main, COLOR_NAME bar_color_highlight);

static bool M_CalcObjectVertices(const XYZ_16 *vertices, int16_t count);
static void M_CalcVerticeLight(const OBJECT_MESH *mesh);
static void M_CalcSkyboxLight(const OBJECT_MESH *mesh);

static void M_InsertBar(
    const int32_t l, const int32_t t, const int32_t w, const int32_t h,
    const int32_t percent, const COLOR_NAME bar_color_main,
    const COLOR_NAME bar_color_highlight)
{
    struct {
        int32_t x1, y1, x2, y2;
        COLOR_NAME color;
    } rects[] = {
        { 0, 0, w, h, COLOR_WHITE },
        { 1, 1, w, h, COLOR_GRAY },
        { 1, 1, w - 1, h - 1, COLOR_BLACK },
        { 2, 2, (w - 2) * percent / 100, h - 2, bar_color_main },
        { 2, 3, (w - 2) * percent / 100, 4, bar_color_highlight },
    };

    const int32_t z_offset = 8;
    const int32_t x_offset = l < 0
        ? g_PhdWinWidth + Scaler_Calc(l, SCALER_TARGET_GENERIC)
            - Scaler_Calc(w - 1, SCALER_TARGET_BAR)
        : Scaler_Calc(l, SCALER_TARGET_GENERIC);
    const int32_t y_offset = t < 0
        ? g_PhdWinHeight + Scaler_Calc(t, SCALER_TARGET_GENERIC)
            - Scaler_Calc(h - 1, SCALER_TARGET_BAR)
        : Scaler_Calc(t, SCALER_TARGET_GENERIC);

    for (int32_t i = 0; i < 5; i++) {
        Render_InsertFlatRect(
            x_offset + Scaler_Calc(rects[i].x1, SCALER_TARGET_BAR),
            y_offset + Scaler_Calc(rects[i].y1, SCALER_TARGET_BAR),
            x_offset + Scaler_Calc(rects[i].x2, SCALER_TARGET_BAR),
            y_offset + Scaler_Calc(rects[i].y2, SCALER_TARGET_BAR),
            g_PhdNearZ + z_offset * (5 - i),
            m_NamedColors[rects[i].color].palette_index);
    }
}

static void M_CalcRoomVertices(const ROOM_MESH *const mesh, int32_t far_clip)
{
    const double base_z = g_Config.rendering.enable_zbuffer
        ? 0.0
        : (g_MidSort << (W2V_SHIFT + 8));

    for (int32_t i = 0; i < mesh->num_vertices; i++) {
        PHD_VBUF *const vbuf = &g_PhdVBuf[i];
        const ROOM_VERTEX *const vertex = &mesh->vertices[i];

        // clang-format off
        const MATRIX *const mptr = g_MatrixPtr;
        const double xv = (
            mptr->_00 * vertex->pos.x +
            mptr->_01 * vertex->pos.y +
            mptr->_02 * vertex->pos.z +
            mptr->_03
        );
        const double yv = (
            mptr->_10 * vertex->pos.x +
            mptr->_11 * vertex->pos.y +
            mptr->_12 * vertex->pos.z +
            mptr->_13
        );
        const int32_t zv_int = (
            mptr->_20 * vertex->pos.x +
            mptr->_21 * vertex->pos.y +
            mptr->_22 * vertex->pos.z +
            mptr->_23
        );
        const double zv = zv_int;
        // clang-format on

        vbuf->xv = xv;
        vbuf->yv = yv;
        vbuf->zv = zv;

        int16_t shade = vertex->light_adder;
        if (g_IsWaterEffect) {
            shade += m_ShadesTable
                [((uint8_t)g_WibbleOffset
                  + (uint8_t)
                      m_RandomTable[(mesh->num_vertices - i) % WIBBLE_SIZE])
                 % WIBBLE_SIZE];
        }

        uint16_t clip_flags = 0;
        if (zv < g_FltNearZ) {
            clip_flags = 0xFF80;
        } else {
            const double persp = g_FltPersp / zv;
            const int32_t depth = zv_int >> W2V_SHIFT;
            vbuf->zv += base_z;

            if (depth < FOG_END) {
                if (depth > FOG_START) {
                    shade += depth - FOG_START;
                }
                vbuf->rhw = persp * g_FltRhwOPersp;
            } else {
                // clip_flags = far_clip;
                shade = 0x1FFF;
                vbuf->zv = g_FltFarZ;
                vbuf->rhw = persp * g_FltRhwOPersp;
            }

            double xs = xv * persp + g_FltWinCenterX;
            double ys = yv * persp + g_FltWinCenterY;

            if (xs < g_FltWinLeft) {
                clip_flags |= 1;
            } else if (xs > g_FltWinRight) {
                clip_flags |= 2;
            }

            if (ys < g_FltWinTop) {
                clip_flags |= 4;
            } else if (ys > g_FltWinBottom) {
                clip_flags |= 8;
            }

            vbuf->xs = xs;
            vbuf->ys = ys;
            // clip_flags |= (~((uint8_t)(vbuf->zv / 0x155555.p0))) << 8;
        }

        CLAMP(shade, 0, 0x1FFF);
        vbuf->g = shade;
        vbuf->clip = clip_flags;
    }
}

static void M_CalcRoomVerticesWibble(const ROOM_MESH *const mesh)
{
    for (int32_t i = 0; i < mesh->num_vertices; i++) {
        const ROOM_VERTEX *const vertex = &mesh->vertices[i];
        if (vertex->flags != 0) {
            continue;
        }

        PHD_VBUF *const vbuf = &g_PhdVBuf[i];
        double xs = vbuf->xs;
        double ys = vbuf->ys;
        xs += m_WibbleTable
            [(((g_WibbleOffset + (int)ys) % WIBBLE_SIZE) + WIBBLE_SIZE)
             % WIBBLE_SIZE];
        ys += m_WibbleTable
            [(((g_WibbleOffset + (int)xs) % WIBBLE_SIZE) + WIBBLE_SIZE)
             % WIBBLE_SIZE];

        int16_t clip_flags = vbuf->clip & ~15;
        if (xs < g_FltWinLeft) {
            clip_flags |= 1;
        } else if (xs > g_FltWinRight) {
            clip_flags |= 2;
        }

        if (ys < g_FltWinTop) {
            clip_flags |= 4;
        } else if (ys > g_FltWinBottom) {
            clip_flags |= 8;
        }
        vbuf->xs = xs;
        vbuf->ys = ys;
        vbuf->clip = clip_flags;
    }
}

static bool M_CalcObjectVertices(
    const XYZ_16 *const vertices, const int16_t count)
{
    const double base_z = g_Config.rendering.enable_zbuffer
        ? 0.0
        : (g_MidSort << (W2V_SHIFT + 8));
    uint8_t total_clip = 0xFF;

    for (int32_t i = 0; i < count; i++) {
        PHD_VBUF *const vbuf = &g_PhdVBuf[i];
        const XYZ_16 *const vertex = &vertices[i];

        // clang-format off
        const MATRIX *const mptr = g_MatrixPtr;
        const double xv = (
            mptr->_00 * vertex->x +
            mptr->_01 * vertex->y +
            mptr->_02 * vertex->z +
            mptr->_03
        );
        const double yv = (
            mptr->_10 * vertex->x +
            mptr->_11 * vertex->y +
            mptr->_12 * vertex->z +
            mptr->_13
        );
        double zv = (
            mptr->_20 * vertex->x +
            mptr->_21 * vertex->y +
            mptr->_22 * vertex->z +
            mptr->_23
        );
        // clang-format on

        vbuf->xv = xv;
        vbuf->yv = yv;

        uint8_t clip_flags;
        if (zv < g_FltNearZ) {
            vbuf->zv = zv;
            clip_flags = 0x80;
        } else {
            if (zv >= g_FltFarZ) {
                vbuf->zv = g_FltFarZ;
            } else {
                vbuf->zv = zv + base_z;
            }

            const double persp = g_FltPersp / zv;
            vbuf->xs = persp * xv + g_FltWinCenterX;
            vbuf->ys = persp * yv + g_FltWinCenterY;
            vbuf->rhw = persp * g_FltRhwOPersp;

            clip_flags = 0x00;
            if (vbuf->xs < g_FltWinLeft) {
                clip_flags |= 1;
            } else if (vbuf->xs > g_FltWinRight) {
                clip_flags |= 2;
            }

            if (vbuf->ys < g_FltWinTop) {
                clip_flags |= 4;
            } else if (vbuf->ys > g_FltWinBottom) {
                clip_flags |= 8;
            }
        }

        vbuf->clip = clip_flags;
        total_clip &= clip_flags;
    }

    return total_clip == 0;
}

static void M_DrawRoomSprites(const ROOM_MESH *const mesh)
{
    for (int32_t i = 0; i < mesh->num_sprites; i++) {
        const ROOM_SPRITE *const room_sprite = &mesh->sprites[i];
        const PHD_VBUF *vbuf = &g_PhdVBuf[room_sprite->vertex];
        if ((int8_t)vbuf->clip < 0) {
            continue;
        }

        const SPRITE_TEXTURE *const sprite =
            Output_GetSpriteTexture(room_sprite->texture);
        ASSERT(sprite != nullptr);
        const double persp = (double)(vbuf->zv / g_PhdPersp);
        const double x0 =
            g_PhdWinCenterX + (vbuf->xv + (sprite->x0 << W2V_SHIFT)) / persp;
        const double y0 =
            g_PhdWinCenterY + (vbuf->yv + (sprite->y0 << W2V_SHIFT)) / persp;
        const double x1 =
            g_PhdWinCenterX + (vbuf->xv + (sprite->x1 << W2V_SHIFT)) / persp;
        const double y1 =
            g_PhdWinCenterY + (vbuf->yv + (sprite->y1 << W2V_SHIFT)) / persp;
        if (x1 >= g_PhdWinLeft && y1 >= g_PhdWinTop && x0 < g_PhdWinRight
            && y0 < g_PhdWinBottom) {
            Render_InsertSprite(
                vbuf->zv, x0, y0, x1, y1, room_sprite->texture, vbuf->g);
        }
    }
}

static void M_CalcVerticeLight(const OBJECT_MESH *const mesh)
{
    // TODO: refactor
    if (mesh->num_lights > 0) {
        if (g_LsDivider) {
            // clang-format off
            const MATRIX *const mptr = g_MatrixPtr;
            int32_t xv = (
                g_LsVectorView.x * mptr->_00 +
                g_LsVectorView.y * mptr->_10 +
                g_LsVectorView.z * mptr->_20
            ) / g_LsDivider;
            int32_t yv = (
                g_LsVectorView.x * mptr->_01 +
                g_LsVectorView.y * mptr->_11 +
                g_LsVectorView.z * mptr->_21
            ) / g_LsDivider;
            int32_t zv = (
                g_LsVectorView.x * mptr->_02 +
                g_LsVectorView.y * mptr->_12 +
                g_LsVectorView.z * mptr->_22
            ) / g_LsDivider;
            // clang-format on

            for (int32_t i = 0; i < mesh->num_lights; i++) {
                const XYZ_16 *const normal = &mesh->lighting.normals[i];
                int16_t shade = g_LsAdder
                    + ((xv * normal->x + yv * normal->y + zv * normal->z)
                       >> 16);
                CLAMP(shade, 0, 0x1FFF);

                g_PhdVBuf[i].g = shade;
            }
        } else {
            int16_t shade = g_LsAdder;
            CLAMP(shade, 0, 0x1FFF);
            for (int32_t i = 0; i < mesh->num_lights; i++) {
                g_PhdVBuf[i].g = shade;
            }
        }
    } else if (mesh->num_lights < 0) {
        for (int32_t i = 0; i < -mesh->num_lights; i++) {
            int16_t shade = g_LsAdder + mesh->lighting.lights[i];
            CLAMP(shade, 0, 0x1FFF);
            g_PhdVBuf[i].g = shade;
        }
    }
}

static void M_CalcSkyboxLight(const OBJECT_MESH *const mesh)
{
    for (int32_t i = 0; i < ABS(mesh->num_lights); i++) {
        g_PhdVBuf[i].g = 0xFFF;
    }
}

void Output_DrawObjectMesh(const OBJECT_MESH *const mesh, const int32_t clip)
{
    g_FltWinLeft = 0.0f;
    g_FltWinTop = 0.0f;
    g_FltWinRight = g_PhdWinMaxX + 1;
    g_FltWinBottom = g_PhdWinMaxY + 1;
    g_FltWinCenterX = g_PhdWinCenterX;
    g_FltWinCenterY = g_PhdWinCenterY;

    if (!M_CalcObjectVertices(mesh->vertices, mesh->num_vertices)) {
        return;
    }

    M_CalcVerticeLight(mesh);
    Render_InsertTexturedFace4s(
        mesh->tex_face4s, mesh->num_tex_face4s, ST_AVG_Z);
    Render_InsertTexturedFace3s(
        mesh->tex_face3s, mesh->num_tex_face3s, ST_AVG_Z);
    Render_InsertFlatFace4s(mesh->flat_face4s, mesh->num_flat_face4s, ST_AVG_Z);
    Render_InsertFlatFace3s(mesh->flat_face3s, mesh->num_flat_face3s, ST_AVG_Z);
}

void Output_DrawObjectMesh_I(const OBJECT_MESH *const mesh, const int32_t clip)
{
    Matrix_Push();
    Matrix_Interpolate();
    Output_DrawObjectMesh(mesh, clip);
    Matrix_Pop();
}

void Output_DrawRoom(const ROOM_MESH *const mesh, const bool is_outside)
{
    g_FltWinLeft = g_PhdWinLeft;
    g_FltWinTop = g_PhdWinTop;
    g_FltWinRight = g_PhdWinRight + 1;
    g_FltWinBottom = g_PhdWinBottom + 1;
    g_FltWinCenterX = g_PhdWinCenterX;
    g_FltWinCenterY = g_PhdWinCenterY;

    M_CalcRoomVertices(mesh, is_outside ? 0 : 16);

    if (g_IsWibbleEffect) {
        Render_EnableZBuffer(false, true);
        g_DiscardTransparent = true;
        Render_InsertTexturedFace4s(mesh->face4s, mesh->num_face4s, ST_MAX_Z);
        Render_InsertTexturedFace3s(mesh->face3s, mesh->num_face3s, ST_MAX_Z);
        g_DiscardTransparent = false;
        M_CalcRoomVerticesWibble(mesh);
        Render_EnableZBuffer(true, true);
    }

    Render_InsertTexturedFace4s(mesh->face4s, mesh->num_face4s, ST_MAX_Z);
    Render_InsertTexturedFace3s(mesh->face3s, mesh->num_face3s, ST_MAX_Z);

    M_DrawRoomSprites(mesh);
}

void Output_DrawSkybox(const OBJECT_MESH *const mesh)
{
    g_FltWinLeft = g_PhdWinLeft;
    g_FltWinTop = g_PhdWinTop;
    g_FltWinRight = g_PhdWinRight + 1;
    g_FltWinBottom = g_PhdWinBottom + 1;
    g_FltWinCenterX = g_PhdWinCenterX;
    g_FltWinCenterY = g_PhdWinCenterY;

    if (!M_CalcObjectVertices(mesh->vertices, mesh->num_vertices)) {
        return;
    }

    Render_EnableZBuffer(false, false);
    M_CalcSkyboxLight(mesh);
    Render_InsertTexturedFace4s(
        mesh->tex_face4s, mesh->num_tex_face4s, ST_FAR_Z);
    Render_InsertTexturedFace3s(
        mesh->tex_face3s, mesh->num_tex_face3s, ST_FAR_Z);
    Render_InsertFlatFace4s(mesh->flat_face4s, mesh->num_flat_face4s, ST_FAR_Z);
    Render_InsertFlatFace3s(mesh->flat_face3s, mesh->num_flat_face3s, ST_FAR_Z);
    Render_EnableZBuffer(true, true);
}

void Output_RotateLight(int16_t pitch, int16_t yaw)
{
    int32_t xcos = Math_Cos(pitch);
    int32_t xsin = Math_Sin(pitch);
    int32_t wcos = Math_Cos(yaw);
    int32_t wsin = Math_Sin(yaw);

    int32_t x = (xcos * wsin) >> W2V_SHIFT;
    int32_t y = -xsin;
    int32_t z = (xcos * wcos) >> W2V_SHIFT;

    const MATRIX *const m = &g_W2VMatrix;
    g_LsVectorView.x = (m->_00 * x + m->_01 * y + m->_02 * z) >> W2V_SHIFT;
    g_LsVectorView.y = (m->_10 * x + m->_11 * y + m->_12 * z) >> W2V_SHIFT;
    g_LsVectorView.z = (m->_20 * x + m->_21 * y + m->_22 * z) >> W2V_SHIFT;
}

void Output_DrawSprite(
    const uint32_t flags, int32_t x, int32_t y, int32_t z,
    const int16_t sprite_idx, int16_t shade, const int16_t scale)
{
    const MATRIX *const mptr = g_MatrixPtr;

    int32_t xv;
    int32_t yv;
    int32_t zv;
    if (flags & SPRF_ABS) {
        x -= g_W2VMatrix._03;
        y -= g_W2VMatrix._13;
        z -= g_W2VMatrix._23;
        if (x < -g_PhdViewDistance || x > g_PhdViewDistance
            || y < -g_PhdViewDistance || y > g_PhdViewDistance
            || z < -g_PhdViewDistance || z > g_PhdViewDistance) {
            return;
        }

        zv = g_W2VMatrix._22 * z + g_W2VMatrix._21 * y + g_W2VMatrix._20 * x;
        if (zv < g_PhdNearZ || zv >= g_PhdFarZ) {
            return;
        }
        xv = g_W2VMatrix._02 * z + g_W2VMatrix._01 * y + g_W2VMatrix._00 * x;
        yv = g_W2VMatrix._12 * z + g_W2VMatrix._11 * y + g_W2VMatrix._10 * x;
    } else if (x | y | z) {
        zv = x * mptr->_20 + y * mptr->_21 + z * mptr->_22 + mptr->_23;
        if (zv < g_PhdNearZ || zv > g_PhdFarZ) {
            return;
        }
        xv = x * mptr->_00 + y * mptr->_01 + z * mptr->_02 + mptr->_03;
        yv = x * mptr->_10 + y * mptr->_11 + z * mptr->_12 + mptr->_13;
    } else {
        zv = mptr->_23;
        if (zv < g_PhdNearZ || zv > g_PhdFarZ) {
            return;
        }
        xv = mptr->_03;
        yv = mptr->_13;
    }

    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);
    ASSERT(sprite != nullptr);
    int32_t x0 = sprite->x0;
    int32_t y0 = sprite->y0;
    int32_t x1 = sprite->x1;
    int32_t y1 = sprite->y1;

    if (flags & SPRF_SCALE) {
        x0 = (x0 * scale) << (W2V_SHIFT - 8);
        y0 = (y0 * scale) << (W2V_SHIFT - 8);
        x1 = (x1 * scale) << (W2V_SHIFT - 8);
        y1 = (y1 * scale) << (W2V_SHIFT - 8);
    } else {
        x0 <<= W2V_SHIFT;
        y0 <<= W2V_SHIFT;
        x1 <<= W2V_SHIFT;
        y1 <<= W2V_SHIFT;
    }

    int32_t zp = zv / g_PhdPersp;

    x0 = g_PhdWinCenterX + (x0 + xv) / zp;
    if (x0 >= g_PhdWinWidth) {
        return;
    }

    y0 = g_PhdWinCenterY + (y0 + yv) / zp;
    if (y0 >= g_PhdWinHeight) {
        return;
    }

    x1 = g_PhdWinCenterX + (x1 + xv) / zp;
    if (x1 < 0) {
        return;
    }

    y1 = g_PhdWinCenterY + (y1 + yv) / zp;
    if (y1 < 0) {
        return;
    }

    if (flags & SPRF_SHADE) {
        const int32_t depth = zv >> W2V_SHIFT;
        if (depth > FOG_START) {
            shade += depth - FOG_START;
            if (shade > 0x1FFF) {
                return;
            }
        }
    } else {
        shade = 0x1000;
    }

    Render_InsertSprite(zv, x0, y0, x1, y1, sprite_idx, shade);
}

void Output_DrawPickup(
    const int32_t sx, const int32_t sy, const int32_t scale,
    const int16_t sprite_idx, const int16_t shade)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);
    ASSERT(sprite != nullptr);
    const int32_t x0 = sx + ((sprite->x0 * scale) / PHD_ONE);
    const int32_t y0 = sy + ((sprite->y0 * scale) / PHD_ONE);
    const int32_t x1 = sx + ((sprite->x1 * scale) / PHD_ONE);
    const int32_t y1 = sy + ((sprite->y1 * scale) / PHD_ONE);
    if (x1 >= 0 && y1 >= 0 && x0 < g_PhdWinWidth && y0 < g_PhdWinHeight) {
        Render_InsertSprite(200, x0, y0, x1, y1, sprite_idx, shade);
    }
}

void Output_DrawScreenSprite(
    const int32_t sx, const int32_t sy, const int32_t sz, const int32_t scale_h,
    const int32_t scale_v, const int16_t sprite_idx, const int16_t shade,
    const uint16_t flags)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);
    ASSERT(sprite != nullptr);
    const int32_t x0 = sx + ((sprite->x0 * scale_h) / PHD_ONE);
    const int32_t y0 = sy + ((sprite->y0 * scale_v) / PHD_ONE);
    const int32_t x1 = sx + ((sprite->x1 * scale_h) / PHD_ONE);
    const int32_t y1 = sy + ((sprite->y1 * scale_v) / PHD_ONE);
    const int32_t z = g_PhdNearZ + sz * 8;
    if (x1 >= 0 && y1 >= 0 && x0 < g_PhdWinWidth && y0 < g_PhdWinHeight) {
        Render_InsertSprite(z, x0, y0, x1, y1, sprite_idx, shade);
    }
}

bool Output_MakeScreenshot(const char *const path)
{
    LOG_INFO("Taking screenshot");
    GFX_Context_ScheduleScreenshot(path);
    return true;
}

void Output_BeginScene(void)
{
    Matrix_ResetStack();
    Text_DrawReset();
    Render_BeginScene();
}

void Output_EndScene(void)
{
    Render_EndScene();
    Shell_ProcessEvents();
}

BACKGROUND_TYPE Output_GetBackgroundType(void)
{
    return m_BackgroundType;
}

bool Output_LoadBackgroundFromFile(const char *const file_name)
{
    IMAGE *const image = Image_CreateFromFile(file_name);
    if (image == nullptr) {
        return false;
    }
    Render_LoadBackgroundFromImage(image);
    Image_Free(image);
    m_BackgroundType = BK_IMAGE;
    return true;
}

void Output_LoadBackgroundFromObject(void)
{
    const OBJECT *const obj = Object_Get(O_INV_BACKGROUND);
    if (!obj->loaded) {
        return;
    }

    const OBJECT_MESH *const mesh = Object_GetMesh(obj->mesh_idx);
    if (mesh->num_tex_face4s < 1) {
        return;
    }

    const int32_t texture_idx = mesh->tex_face4s[0].texture_idx;
    const OBJECT_TEXTURE *const texture = Output_GetObjectTexture(texture_idx);
    ASSERT(texture != nullptr);
    Render_LoadBackgroundFromTexture(texture, 8, 6);
    m_BackgroundType = BK_OBJECT;
    return;
}

void Output_UnloadBackground(void)
{
    Render_UnloadBackground();
    m_BackgroundType = BK_TRANSPARENT;
}

void Output_InsertBackPolygon(
    const int32_t x0, const int32_t y0, const int32_t x1, const int32_t y1)
{
    Render_InsertFlatRect(
        x0, y0, x1, y1, g_PhdFarZ + 1,
        m_NamedColors[COLOR_BLACK].palette_index);
}

void Output_DrawBlackRectangle(int32_t opacity)
{
    Render_DrawBlackRectangle(opacity);
}

void Output_DrawBackground(void)
{
    Render_DrawBackground();
}

void Output_DrawPolyList(void)
{
    Render_DrawPolyList();
}

void Output_DrawScreenLine(
    const int32_t x, const int32_t y, const int32_t z, const int32_t x_len,
    const int32_t y_len, const uint8_t color_idx, const void *const gour,
    const uint16_t flags)
{
    Render_InsertLine(
        x, y, x + x_len, y + y_len, g_PhdNearZ + 8 * z, color_idx);
}

void Output_DrawScreenBox(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t width,
    const int32_t height, const uint8_t color_idx, const void *const gour,
    const uint16_t flags)
{
    const int32_t col_1 = 15;
    const int32_t col_2 = 31;
    Output_DrawScreenLine(sx, sy - 1, z, width + 1, 0, col_1, nullptr, flags);
    Output_DrawScreenLine(sx + 1, sy, z, width - 1, 0, col_2, nullptr, flags);
    Output_DrawScreenLine(
        sx + width, sy + 1, z, 0, height - 1, col_1, nullptr, flags);
    Output_DrawScreenLine(
        sx + width + 1, sy, z, 0, height + 1, col_2, nullptr, flags);
    Output_DrawScreenLine(
        sx - 1, sy - 1, z, 0, height + 1, col_1, nullptr, flags);
    Output_DrawScreenLine(sx, sy, z, 0, height - 1, col_2, nullptr, flags);
    Output_DrawScreenLine(
        sx, height + sy, z, width - 1, 0, col_1, nullptr, flags);
    Output_DrawScreenLine(
        sx - 1, height + sy + 1, z, width + 1, 0, col_2, nullptr, flags);
}

void Output_DrawScreenFBox(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t width,
    const int32_t height, const uint8_t color_idx, const void *const gour,
    const uint16_t flags)
{
    Render_InsertTransQuad(sx, sy, width + 1, height + 1, g_PhdNearZ + 8 * z);
}

void Output_DrawHealthBar(const int32_t percent)
{
    g_IsShadeEffect = false;
    M_InsertBar(8, 8, 105, 9, percent, COLOR_RED, COLOR_ORANGE);
}

void Output_DrawAirBar(const int32_t percent)
{
    g_IsShadeEffect = false;
    M_InsertBar(-8, 8, 105, 9, percent, COLOR_BLUE, COLOR_WHITE);
}

void Output_InsertShadow(
    int16_t radius, const BOUNDS_16 *bounds, const ITEM *item)
{
    const int32_t x1 = bounds->min.x;
    const int32_t x2 = bounds->max.x;
    const int32_t z1 = bounds->min.z;
    const int32_t z2 = bounds->max.z;
    const int32_t mid_x = (x1 + x2) / 2;
    const int32_t mid_z = (z1 + z2) / 2;
    const int32_t x_add = radius * (x2 - x1) / 1024;
    const int32_t z_add = radius * (z2 - z1) / 1024;

    struct {
        int16_t poly_count;
        int16_t vertex_count;
        XYZ_16 vertex[8];
    } shadow_info = {
        .poly_count = 1,
        .vertex_count = 8,
        .vertex = {
            { .x = mid_x - x_add, .z = mid_z + z_add * 2 },
            { .x = mid_x + x_add, .z = mid_z + z_add * 2 },
            { .x = mid_x + x_add * 2, .z = z_add + mid_z },
            { .x = mid_x + x_add * 2, .z = mid_z - z_add },
            { .x = mid_x + x_add, .z = mid_z - 2 * z_add },
            { .x = mid_x - x_add, .z = mid_z - 2 * z_add },
            { .x = mid_x - 2 * x_add, .z = mid_z - z_add },
            { .x = mid_x - 2 * x_add, .z = z_add + mid_z },
        },
    };

    g_FltWinLeft = (float)(g_PhdWinLeft);
    g_FltWinTop = (float)(g_PhdWinTop);
    g_FltWinRight = (float)(g_PhdWinRight + 1);
    g_FltWinBottom = (float)(g_PhdWinBottom + 1);
    g_FltWinCenterX = (float)(g_PhdWinCenterX);
    g_FltWinCenterY = (float)(g_PhdWinCenterY);

    Matrix_Push();
    Matrix_TranslateAbs(item->pos.x, item->floor, item->pos.z);
    Matrix_RotY(item->rot.y);
    if (M_CalcObjectVertices(shadow_info.vertex, shadow_info.vertex_count)) {
        Render_InsertTransOctagon(g_PhdVBuf, 24);
    }
    Matrix_Pop();
}

void Output_CalculateWibbleTable(void)
{
    for (int32_t i = 0; i < WIBBLE_SIZE; i++) {
        const int32_t sine = Math_Sin(i * DEG_360 / WIBBLE_SIZE);
        m_WibbleTable[i] = (sine * MAX_WIBBLE) >> W2V_SHIFT;
        m_ShadesTable[i] = (sine * MAX_SHADE) >> W2V_SHIFT;
        m_RandomTable[i] = (Random_GetDraw() >> 5) - 0x01FF;
        for (int32_t j = 0; j < WIBBLE_SIZE; j++) {
            m_RoomLightTables[i].table[j] = (j - (WIBBLE_SIZE / 2)) * i
                * MAX_ROOM_LIGHT_UNIT / (WIBBLE_SIZE - 1);
        }
    }
}

int32_t Output_GetObjectBounds(const BOUNDS_16 *const bounds)
{
    const MATRIX *const m = g_MatrixPtr;
    if (m->_23 >= g_PhdFarZ) {
        return 0;
    }

    const int32_t vtx_count = 8;
    const XYZ_32 vtx[] = {
        { .x = bounds->min.x, .y = bounds->min.y, .z = bounds->min.z },
        { .x = bounds->max.x, .y = bounds->min.y, .z = bounds->min.z },
        { .x = bounds->max.x, .y = bounds->max.y, .z = bounds->min.z },
        { .x = bounds->min.x, .y = bounds->max.y, .z = bounds->min.z },
        { .x = bounds->min.x, .y = bounds->min.y, .z = bounds->max.z },
        { .x = bounds->max.x, .y = bounds->min.y, .z = bounds->max.z },
        { .x = bounds->max.x, .y = bounds->max.y, .z = bounds->max.z },
        { .x = bounds->min.x, .y = bounds->max.y, .z = bounds->max.z },
    };

    int32_t y_min = 0x3FFFFFFF;
    int32_t x_min = 0x3FFFFFFF;
    int32_t y_max = -0x3FFFFFFF;
    int32_t x_max = -0x3FFFFFFF;

    int32_t num_z = 0;
    for (int32_t i = 0; i < vtx_count; i++) {
        const int32_t x = vtx[i].x;
        const int32_t y = vtx[i].y;
        const int32_t z = vtx[i].z;

        const int32_t zv = x * m->_20 + y * m->_21 + z * m->_22 + m->_23;
        if (zv <= g_PhdNearZ || zv >= g_PhdFarZ) {
            continue;
        }

        num_z++;
        const int32_t zp = zv / g_PhdPersp;
        const int32_t xv = (x * m->_00 + y * m->_01 + z * m->_02 + m->_03) / zp;
        const int32_t yv = (x * m->_10 + y * m->_11 + z * m->_12 + m->_13) / zp;
        CLAMPG(x_min, xv);
        CLAMPL(x_max, xv);
        CLAMPG(y_min, yv);
        CLAMPL(y_max, yv);
    }

    x_min += g_PhdWinCenterX;
    x_max += g_PhdWinCenterX;
    y_min += g_PhdWinCenterY;
    y_max += g_PhdWinCenterY;

    if (num_z == 0 || x_min > g_PhdWinRight || y_min > g_PhdWinBottom
        || x_max < g_PhdWinLeft || y_max < g_PhdWinTop) {
        // out of screen
        return 0;
    }

    if (num_z < 8 || x_min < 0 || y_min < 0 || x_max > g_PhdWinMaxX
        || y_max > g_PhdWinMaxY) {
        // clipped
        return -1;
    }

    // fully on screen
    return 1;
}

void Output_SetupBelowWater(const bool is_underwater)
{
    Render_SetWet(is_underwater);
    g_IsWaterEffect = true;
    g_IsWibbleEffect = !is_underwater;
    g_IsShadeEffect = true;
}

void Output_SetupAboveWater(const bool is_underwater)
{
    g_IsWibbleEffect = is_underwater;
    g_IsWaterEffect = false;
    g_IsShadeEffect = is_underwater;
}

void Output_AnimateTextures(const int32_t ticks)
{
    g_WibbleOffset = (g_WibbleOffset + (ticks / TICKS_PER_FRAME)) % WIBBLE_SIZE;
    m_RoomLightShades[RLM_FLICKER] = Random_GetDraw() % WIBBLE_SIZE;
    m_RoomLightShades[RLM_GLOW] = (WIBBLE_SIZE - 1)
            * (Math_Sin((g_WibbleOffset * DEG_360) / WIBBLE_SIZE) + 0x4000)
        >> 15;

    if (g_GF_SunsetEnabled) {
        g_SunsetTimer += ticks;
        CLAMPG(g_SunsetTimer, SUNSET_TIMEOUT);
        m_RoomLightShades[RLM_SUNSET] =
            g_SunsetTimer * (WIBBLE_SIZE - 1) / SUNSET_TIMEOUT;
    }

    m_TickComp += ticks;
    while (m_TickComp > TICKS_PER_FRAME * 5) {
        Output_CycleAnimatedTextures();
        m_TickComp -= TICKS_PER_FRAME * 5;
    }
}

void Output_SetLightAdder(const int32_t adder)
{
    g_LsAdder = adder;
}

void Output_SetLightDivider(const int32_t divider)
{
    g_LsDivider = divider;
}

int32_t Output_CalcFogShade(const int32_t depth)
{
    if (depth > FOG_START) {
        return depth - FOG_START;
    }
    if (depth > FOG_END) {
        return 0x1FFF;
    }
    return 0;
}

int32_t Output_GetRoomLightShade(const ROOM_LIGHT_MODE mode)
{
    return m_RoomLightShades[mode];
}

void Output_LightRoomVertices(const ROOM *const room)
{
    const ROOM_LIGHT_TABLE *const light_table =
        &m_RoomLightTables[m_RoomLightShades[room->light_mode]];
    for (int32_t i = 0; i < room->mesh.num_vertices; i++) {
        ROOM_VERTEX *const vtx = &room->mesh.vertices[i];
        const int32_t wibble =
            light_table->table[vtx->light_table_value % WIBBLE_SIZE];
        vtx->light_adder = vtx->light_base + wibble;
    }
}

void Output_InitialiseNamedColors(void)
{
    for (int32_t i = 0; i < COLOR_NUMBER_OF; i++) {
        m_NamedColors[i].palette_index =
            Output_FindColor8(m_NamedColors[i].rgb);
    }
}
