#include "game/output.h"

#include "config.h"
#include "game/clock.h"
#include "game/console/common.h"
#include "game/gamebuf.h"
#include "game/overlay.h"
#include "game/phase/phase.h"
#include "game/random.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/math.h"
#include "math/math_misc.h"
#include "math/matrix.h"
#include "specific/s_output.h"
#include "specific/s_shell.h"

#include <libtrx/engine/image.h>
#include <libtrx/filesystem.h>
#include <libtrx/game/console/common.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>

#include <stddef.h>
#include <string.h>

#define MAX_LIGHTNINGS 64
#define PHD_IONE (PHD_ONE / 4)

typedef struct {
    struct {
        XYZ_32 pos;
        int32_t thickness;
    } edges[2];
} LIGHTNING;

static bool m_IsSkyboxEnabled = false;
static bool m_IsWibbleEffect = false;
static bool m_IsWaterEffect = false;
static bool m_IsShadeEffect = false;
static int m_OverlayCurAlpha = 0;
static int m_OverlayDstAlpha = 0;
static int m_BackdropCurAlpha = 0;
static int m_BackdropDstAlpha = 0;
static RGB_888 *m_ColorPalette = NULL;

static int32_t m_WibbleOffset = 0;
static double m_WibbleOffsetDbl = 0.0;
static CLOCK_TIMER m_WibbleTimer = { 0 };
static CLOCK_TIMER m_AnimatedTexturesTimer = { 0 };
static CLOCK_TIMER m_FadeTimer = { 0 };
static int32_t m_WibbleTable[WIBBLE_SIZE] = { 0 };
static int32_t m_ShadeTable[WIBBLE_SIZE] = { 0 };
static int32_t m_RandTable[WIBBLE_SIZE] = { 0 };

static PHD_VBUF *m_VBuf = NULL;
static PHD_UV *m_EnvMapUV = NULL;
static int32_t m_DrawDistFade = 0;
static int32_t m_DrawDistMax = 0;
static RGB_F m_WaterColor = { 0 };
static XYZ_32 m_LsVectorView = { 0 };

static int32_t m_LightningCount = 0;
static LIGHTNING m_LightningTable[MAX_LIGHTNINGS];

static char *m_BackdropImagePath = NULL;
static const char *m_ImageExtensions[] = {
    ".png", ".jpg", ".jpeg", ".pcx", NULL,
};

static void M_DrawBlackOverlay(uint8_t alpha);

static const int16_t *M_DrawObjectG3(const int16_t *obj_ptr, int32_t number);
static const int16_t *M_DrawObjectG4(const int16_t *obj_ptr, int32_t number);
static const int16_t *M_DrawObjectGT3(const int16_t *obj_ptr, int32_t number);
static const int16_t *M_DrawObjectGT4(const int16_t *obj_ptr, int32_t number);
static const int16_t *M_DrawObjectEnvMap(
    const int16_t *obj_ptr, int32_t poly_count, int32_t vertex_count,
    bool textured);
static const int16_t *M_DrawRoomSprites(
    const int16_t *obj_ptr, int32_t vertex_count);
static const int16_t *M_CalcObjectVertices(const int16_t *obj_ptr);
static const int16_t *M_CalcVerticeLight(const int16_t *obj_ptr);
static const int16_t *M_CalcVerticeEnvMap(const int16_t *obj_ptr);
static const int16_t *M_CalcSkyboxLight(const int16_t *obj_ptr);
static const int16_t *M_CalcRoomVertices(const int16_t *obj_ptr);
static int32_t M_CalcFogShade(int32_t depth);
static void M_CalcWibbleTable(void);

static const int16_t *M_DrawObjectG3(const int16_t *obj_ptr, int32_t number)
{
    S_Output_DisableTextureMode();

    for (int32_t i = 0; i < number; i++) {
        PHD_VBUF *const vns[3] = {
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
        };
        const int16_t color_idx = DISABLE_REFLECTION_BIT(*obj_ptr++);
        const RGB_888 color = Output_GetPaletteColor(color_idx);
        S_Output_DrawFlatTriangle(vns[0], vns[1], vns[2], color);
    }

    return obj_ptr;
}

static const int16_t *M_DrawObjectG4(const int16_t *obj_ptr, int32_t number)
{
    S_Output_DisableTextureMode();

    for (int32_t i = 0; i < number; i++) {
        PHD_VBUF *const vns[4] = {
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
        };
        const int16_t color_idx = DISABLE_REFLECTION_BIT(*obj_ptr++);

        const RGB_888 color = Output_GetPaletteColor(color_idx);
        S_Output_DrawFlatTriangle(vns[0], vns[1], vns[2], color);
        S_Output_DrawFlatTriangle(vns[2], vns[3], vns[0], color);
    }

    return obj_ptr;
}

static const int16_t *M_DrawObjectGT3(const int16_t *obj_ptr, int32_t number)
{
    S_Output_EnableTextureMode();

    for (int32_t i = 0; i < number; i++) {
        PHD_VBUF *const vns[3] = {
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
        };
        const int16_t texture_num = DISABLE_REFLECTION_BIT(*obj_ptr++);
        PHD_TEXTURE *const tex = &g_PhdTextureInfo[texture_num];

        S_Output_DrawTexturedTriangle(
            vns[0], vns[1], vns[2], tex->tpage, &tex->uv[0], &tex->uv[1],
            &tex->uv[2], tex->drawtype);
    }

    return obj_ptr;
}

static const int16_t *M_DrawObjectGT4(const int16_t *obj_ptr, int32_t number)
{
    S_Output_EnableTextureMode();

    for (int32_t i = 0; i < number; i++) {
        PHD_VBUF *const vns[4] = {
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
            &m_VBuf[*obj_ptr++],
        };
        const int16_t texture_num = DISABLE_REFLECTION_BIT(*obj_ptr++);
        PHD_TEXTURE *const tex = &g_PhdTextureInfo[texture_num];

        S_Output_DrawTexturedQuad(
            vns[0], vns[1], vns[2], vns[3], tex->tpage, &tex->uv[0],
            &tex->uv[1], &tex->uv[2], &tex->uv[3], tex->drawtype);
    }

    return obj_ptr;
}

static const int16_t *M_DrawObjectEnvMap(
    const int16_t *obj_ptr, const int32_t poly_count,
    const int32_t vertex_count, const bool textured)
{
    const PHD_VBUF *vns[4];
    const PHD_UV *uv[4];

    for (int32_t i = 0; i < poly_count; i++) {
        for (int32_t j = 0; j < vertex_count; j++) {
            const int16_t vertex_num = *obj_ptr++;
            vns[j] = &m_VBuf[vertex_num];
            uv[j] = &m_EnvMapUV[vertex_num];
        }

        const int16_t color_or_texture_num = *obj_ptr++;
        if (!IS_REFLECTION_ENABLED(color_or_texture_num)) {
            continue;
        }

        if (vertex_count == 3) {
            S_Output_DrawEnvMapTriangle(
                vns[0], vns[1], vns[2], uv[0], uv[1], uv[2]);
        } else if (vertex_count == 4) {
            S_Output_DrawEnvMapQuad(
                vns[0], vns[1], vns[2], vns[3], uv[0], uv[1], uv[2], uv[3]);
        }
    }

    return obj_ptr;
}

static const int16_t *M_DrawRoomSprites(
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
            Viewport_GetCenterX() + (vbuf->xv + (sprite->x1 << W2V_SHIFT)) / zp;
        int32_t y1 =
            Viewport_GetCenterY() + (vbuf->yv + (sprite->y1 << W2V_SHIFT)) / zp;
        int32_t x2 =
            Viewport_GetCenterX() + (vbuf->xv + (sprite->x2 << W2V_SHIFT)) / zp;
        int32_t y2 =
            Viewport_GetCenterY() + (vbuf->yv + (sprite->y2 << W2V_SHIFT)) / zp;
        if (x2 >= g_PhdLeft && y2 >= g_PhdTop && x1 < g_PhdRight
            && y1 < g_PhdBottom) {
            S_Output_DrawSprite(x1, y1, x2, y2, zv, sprnum, vbuf->g);
        }
    }

    return obj_ptr;
}

static const int16_t *M_CalcObjectVertices(const int16_t *obj_ptr)
{
    int16_t total_clip = -1;

    obj_ptr++;
    int vertex_count = *obj_ptr++;
    for (int i = 0; i < vertex_count; i++) {
        // clang-format off
        double xv =
            g_MatrixPtr->_00 * obj_ptr[0] +
            g_MatrixPtr->_01 * obj_ptr[1] +
            g_MatrixPtr->_02 * obj_ptr[2] +
            g_MatrixPtr->_03;
        double yv =
            g_MatrixPtr->_10 * obj_ptr[0] +
            g_MatrixPtr->_11 * obj_ptr[1] +
            g_MatrixPtr->_12 * obj_ptr[2] +
            g_MatrixPtr->_13;
        double zv =
            g_MatrixPtr->_20 * obj_ptr[0] +
            g_MatrixPtr->_21 * obj_ptr[1] +
            g_MatrixPtr->_22 * obj_ptr[2] +
            g_MatrixPtr->_23;
        // clang-format on

        m_VBuf[i].xv = xv;
        m_VBuf[i].yv = yv;
        m_VBuf[i].zv = zv;

        int16_t clip_flags;
        if (zv < Output_GetNearZ()) {
            clip_flags = -32768;
        } else {
            clip_flags = 0;

            double persp = g_PhdPersp / zv;
            double xs = Viewport_GetCenterX() + xv * persp;
            double ys = Viewport_GetCenterY() + yv * persp;

            if (xs < g_PhdLeft) {
                clip_flags |= 1;
            } else if (xs > g_PhdRight) {
                clip_flags |= 2;
            }

            if (ys < g_PhdTop) {
                clip_flags |= 4;
            } else if (ys > g_PhdBottom) {
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

static const int16_t *M_CalcVerticeLight(const int16_t *obj_ptr)
{
    int32_t vertex_count = *obj_ptr++;
    if (vertex_count > 0) {
        if (g_LsDivider) {
            int32_t xv = (g_MatrixPtr->_00 * m_LsVectorView.x
                          + g_MatrixPtr->_10 * m_LsVectorView.y
                          + g_MatrixPtr->_20 * m_LsVectorView.z)
                / g_LsDivider;
            int32_t yv = (g_MatrixPtr->_01 * m_LsVectorView.x
                          + g_MatrixPtr->_11 * m_LsVectorView.y
                          + g_MatrixPtr->_21 * m_LsVectorView.z)
                / g_LsDivider;
            int32_t zv = (g_MatrixPtr->_02 * m_LsVectorView.x
                          + g_MatrixPtr->_12 * m_LsVectorView.y
                          + g_MatrixPtr->_22 * m_LsVectorView.z)
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

static const int16_t *M_CalcVerticeEnvMap(const int16_t *obj_ptr)
{
    const int32_t vtx_count = *obj_ptr++;
    if (vtx_count <= 0) {
        return NULL;
    }

    for (int32_t i = 0; i < vtx_count; ++i) {
        // make sure that reflection will be drawn after normal poly
        m_VBuf[i].zv -= (double)(W2V_SCALE / 2);

        // set lighting that depends only from fog distance
        m_VBuf[i].g = 0x1000;

        const int32_t depth = g_MatrixPtr->_23 >> W2V_SHIFT;
        m_VBuf[i].g += M_CalcFogShade(depth);

        // reflection can be darker but not brighter
        CLAMP(m_VBuf[i].g, 0x1000, 0x1FFF);

        // rotate normal vectors for X/Y, no translation

        // clang-format off
        int32_t x = (
            g_MatrixPtr->_00 * obj_ptr[0] +
            g_MatrixPtr->_01 * obj_ptr[1] +
            g_MatrixPtr->_02 * obj_ptr[2]
        ) >> W2V_SHIFT;

        int32_t y = (
            g_MatrixPtr->_10 * obj_ptr[0] +
            g_MatrixPtr->_11 * obj_ptr[1] +
            g_MatrixPtr->_12 * obj_ptr[2]
        ) >> W2V_SHIFT;
        // clang-format on

        CLAMP(x, -PHD_ONE, PHD_IONE);
        CLAMP(y, -PHD_ONE, PHD_IONE);
        m_EnvMapUV[i].u = PHD_ONE / PHD_IONE * (x + PHD_IONE) / 2;
        m_EnvMapUV[i].v = PHD_ONE - PHD_ONE / PHD_IONE * (y + PHD_IONE) / 2;
        obj_ptr += 3;
    }
    return obj_ptr;
}

static const int16_t *M_CalcSkyboxLight(const int16_t *obj_ptr)
{
    int32_t vertex_count = *obj_ptr++;
    if (vertex_count > 0) {
        obj_ptr += 3 * vertex_count;
    } else {
        vertex_count = -vertex_count;
        obj_ptr += vertex_count;
    }

    for (int i = 0; i < vertex_count; i++) {
        m_VBuf[i].g = 0xFFF;
    }

    return obj_ptr;
}

static const int16_t *M_CalcRoomVertices(const int16_t *obj_ptr)
{
    int32_t vertex_count = *obj_ptr++;

    for (int i = 0; i < vertex_count; i++) {
        PHD_VBUF *const vbuf = &m_VBuf[i];

        // clang-format off
        const double xv = (
            g_MatrixPtr->_00 * obj_ptr[0] +
            g_MatrixPtr->_01 * obj_ptr[1] +
            g_MatrixPtr->_02 * obj_ptr[2] +
            g_MatrixPtr->_03
        );
        const double yv = (
            g_MatrixPtr->_10 * obj_ptr[0] +
            g_MatrixPtr->_11 * obj_ptr[1] +
            g_MatrixPtr->_12 * obj_ptr[2] +
            g_MatrixPtr->_13
        );
        const int32_t zv_int = (
            g_MatrixPtr->_20 * obj_ptr[0] +
            g_MatrixPtr->_21 * obj_ptr[1] +
            g_MatrixPtr->_22 * obj_ptr[2] +
            g_MatrixPtr->_23
        );
        const double zv = zv_int;
        // clang-format on

        m_VBuf[i].xv = xv;
        m_VBuf[i].yv = yv;
        m_VBuf[i].zv = zv;
        m_VBuf[i].g = obj_ptr[3] & MAX_LIGHTING;

        if (zv < Output_GetNearZ()) {
            m_VBuf[i].clip = 0x8000;
        } else {
            int16_t clip_flags = 0;
            const int32_t depth = zv_int >> W2V_SHIFT;
            if (depth > Output_GetDrawDistMax()) {
                m_VBuf[i].g = MAX_LIGHTING;
                if (!m_IsSkyboxEnabled) {
                    clip_flags |= 16;
                }
            } else if (depth) {
                m_VBuf[i].g += M_CalcFogShade(depth);
                if (!m_IsWaterEffect) {
                    CLAMPG(m_VBuf[i].g, MAX_LIGHTING);
                }
            }

            const double persp = g_PhdPersp / (double)zv;
            double xs = Viewport_GetCenterX() + xv * persp;
            double ys = Viewport_GetCenterY() + yv * persp;
            if (m_IsWibbleEffect && !(obj_ptr[3] & NO_VERT_MOVE)) {
                xs += m_WibbleTable[(m_WibbleOffset + (int)ys) & 0x1F];
                ys += m_WibbleTable[(m_WibbleOffset + (int)xs) & 0x1F];
            }

            if (xs < g_PhdLeft) {
                clip_flags |= 1;
            } else if (xs > g_PhdRight) {
                clip_flags |= 2;
            }

            if (ys < g_PhdTop) {
                clip_flags |= 4;
            } else if (ys > g_PhdBottom) {
                clip_flags |= 8;
            }

            if (m_IsWaterEffect) {
                m_VBuf[i].g += m_ShadeTable[(
                    ((uint8_t)m_WibbleOffset
                     + (uint8_t)m_RandTable[(vertex_count - i) % WIBBLE_SIZE])
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

static int32_t M_CalcFogShade(int32_t depth)
{
    int32_t fog_begin = Output_GetDrawDistFade();
    int32_t fog_end = Output_GetDrawDistMax();

    if (depth < fog_begin) {
        return 0;
    }
    if (depth >= fog_end) {
        return 0x1FFF;
    }

    return (depth - fog_begin) * 0x1FFF / (fog_end - fog_begin);
}

static void M_CalcWibbleTable(void)
{
    for (int i = 0; i < WIBBLE_SIZE; i++) {
        PHD_ANGLE angle = (i * PHD_360) / WIBBLE_SIZE;
        m_WibbleTable[i] = Math_Sin(angle) * MAX_WIBBLE >> W2V_SHIFT;
        m_ShadeTable[i] = Math_Sin(angle) * MAX_SHADE >> W2V_SHIFT;
        m_RandTable[i] = (Random_GetDraw() >> 5) - 0x01FF;
    }
}

bool Output_Init(void)
{
    M_CalcWibbleTable();
    return S_Output_Init();
}

void Output_Shutdown(void)
{
    S_Output_Shutdown();
    Memory_FreePointer(&m_BackdropImagePath);
    Memory_FreePointer(&m_ColorPalette);
}

void Output_ReserveVertexBuffer(const size_t size)
{
    m_VBuf = GameBuf_Alloc(size * sizeof(PHD_VBUF), GBUF_VERTEX_BUFFER);
    m_EnvMapUV = GameBuf_Alloc(size * sizeof(PHD_UV), GBUF_VERTEX_BUFFER);
}

void Output_SetWindowSize(int width, int height)
{
    S_Output_SetWindowSize(width, height);
}

void Output_ApplyRenderSettings(void)
{
    S_Output_ApplyRenderSettings();
    if (m_BackdropImagePath) {
        Output_LoadBackdropImage(m_BackdropImagePath);
    }
}

void Output_DownloadTextures(int page_count)
{
    S_Output_DownloadTextures(page_count);
}

RGBA_8888 Output_RGB2RGBA(const RGB_888 color)
{
    RGBA_8888 ret = { .r = color.r, .g = color.g, .b = color.b, .a = 255 };
    return ret;
}

void Output_DrawBlack(void)
{
    M_DrawBlackOverlay(255);
}

void Output_FlushTranslucentObjects(void)
{
    // draw transparent lightnings as last in the 3D geometry pipeline
    for (int32_t i = 0; i < m_LightningCount; i++) {
        const LIGHTNING *const lightning = &m_LightningTable[i];
        S_Output_DrawLightningSegment(
            lightning->edges[0].pos.x, lightning->edges[0].pos.y,
            lightning->edges[0].pos.z, lightning->edges[0].thickness,
            lightning->edges[1].pos.x, lightning->edges[1].pos.y,
            lightning->edges[1].pos.z, lightning->edges[1].thickness);
    }
}

void Output_BeginScene(void)
{
    S_Output_RenderBegin();
    m_LightningCount = 0;
}

void Output_EndScene(void)
{
    Output_DrawOverlayScreen();
    S_Output_DisableDepthTest();
    S_Output_ClearDepthBuffer();
    Overlay_DrawFPSInfo();
    Console_Draw();
    S_Output_EnableDepthTest();
    S_Output_RenderEnd();
    S_Output_FlipScreen();
    S_Shell_SpinMessageLoop();
    g_FPSCounter++;
}

void Output_ClearDepthBuffer(void)
{
    S_Output_ClearDepthBuffer();
}

void Output_CalculateLight(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    ROOM *r = &g_RoomInfo[room_num];

    if (r->num_lights == 0) {
        g_LsAdder = r->ambient;
        g_LsDivider = 0;
    } else {
        int32_t ambient = 0x1FFF - r->ambient;
        int32_t brightest = 0;

        XYZ_32 ls = {
            .x = 0,
            .y = 0,
            .z = 0,
        };
        for (int i = 0; i < r->num_lights; i++) {
            LIGHT *light = &r->lights[i];
            XYZ_32 lc = {
                .x = x - light->pos.x,
                .y = y - light->pos.y,
                .z = z - light->pos.z,
            };

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
        Math_GetVectorAngles(ls.x, ls.y, ls.z, angles);
        Output_RotateLight(angles[1], angles[0]);
    }

    int32_t distance = g_MatrixPtr->_23 >> W2V_SHIFT;
    g_LsAdder += M_CalcFogShade(distance);
    CLAMPG(g_LsAdder, 0x1FFF);
}

void Output_CalculateStaticLight(int16_t adder)
{
    g_LsAdder = adder - 16 * 256;
    int32_t distance = g_MatrixPtr->_23 >> W2V_SHIFT;
    g_LsAdder += M_CalcFogShade(distance);
    CLAMPG(g_LsAdder, 0x1FFF);
}

void Output_CalculateObjectLighting(
    const ITEM *const item, const BOUNDS_16 *const bounds)
{
    if (item->shade >= 0) {
        Output_CalculateStaticLight(item->shade);
        return;
    }

    Matrix_PushUnit();
    Matrix_RotYXZ(item->rot.y, item->rot.x, item->rot.z);
    Matrix_TranslateRel(
        (bounds->min.x + bounds->max.x) / 2,
        (bounds->min.y + bounds->max.y) / 2,
        (bounds->min.z + bounds->max.z) / 2);

    const XYZ_32 offset = {
        .x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT),
        .y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT),
        .z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT),
    };

    Matrix_Pop();

    Output_CalculateLight(offset.x, offset.y, offset.z, item->room_num);
}

void Output_DrawPolygons(const int16_t *obj_ptr, int clip)
{
    const bool has_reflections = IS_REFLECTION_ENABLED(obj_ptr[3]);
    obj_ptr += 4;

    obj_ptr = M_CalcObjectVertices(obj_ptr);
    if (obj_ptr == NULL) {
        return;
    }

    const int16_t *obj_ptr_old = obj_ptr;
    obj_ptr = M_CalcVerticeLight(obj_ptr);
    obj_ptr = M_DrawObjectGT4(obj_ptr + 1, *obj_ptr);
    obj_ptr = M_DrawObjectGT3(obj_ptr + 1, *obj_ptr);
    obj_ptr = M_DrawObjectG4(obj_ptr + 1, *obj_ptr);
    obj_ptr = M_DrawObjectG3(obj_ptr + 1, *obj_ptr);

    if (has_reflections && g_Config.rendering.enable_reflections) {
        obj_ptr = obj_ptr_old;
        obj_ptr = M_CalcVerticeEnvMap(obj_ptr);
        if (obj_ptr != NULL) {
            obj_ptr = M_DrawObjectEnvMap(obj_ptr + 1, *obj_ptr, 4, true);
            obj_ptr = M_DrawObjectEnvMap(obj_ptr + 1, *obj_ptr, 3, true);
            obj_ptr = M_DrawObjectEnvMap(obj_ptr + 1, *obj_ptr, 4, false);
            obj_ptr = M_DrawObjectEnvMap(obj_ptr + 1, *obj_ptr, 3, false);
        }
    }
}

void Output_DrawPolygons_I(const int16_t *obj_ptr, int32_t clip)
{
    Matrix_Push();
    Matrix_Interpolate();
    Output_DrawPolygons(obj_ptr, clip);
    Matrix_Pop();
}

void Output_SetSkyboxEnabled(const bool enabled)
{
    m_IsSkyboxEnabled = enabled;
}

bool Output_IsSkyboxEnabled(void)
{
    return m_IsSkyboxEnabled;
}

void Output_DrawSkybox(const int16_t *obj_ptr)
{
    g_PhdLeft = Viewport_GetMinX();
    g_PhdTop = Viewport_GetMinY();
    g_PhdRight = Viewport_GetMaxX();
    g_PhdBottom = Viewport_GetMaxY();

    obj_ptr = M_CalcObjectVertices(obj_ptr + 4);
    if (!obj_ptr) {
        return;
    }

    S_Output_DisableDepthTest();
    obj_ptr = M_CalcSkyboxLight(obj_ptr);
    obj_ptr = M_DrawObjectGT4(obj_ptr + 1, *obj_ptr);
    obj_ptr = M_DrawObjectGT3(obj_ptr + 1, *obj_ptr);
    obj_ptr = M_DrawObjectG4(obj_ptr + 1, *obj_ptr);
    obj_ptr = M_DrawObjectG3(obj_ptr + 1, *obj_ptr);
    S_Output_EnableDepthTest();
}

void Output_DrawRoom(const int16_t *obj_ptr)
{
    obj_ptr = M_CalcRoomVertices(obj_ptr);
    obj_ptr = M_DrawObjectGT4(obj_ptr + 1, *obj_ptr);
    obj_ptr = M_DrawObjectGT3(obj_ptr + 1, *obj_ptr);
    obj_ptr = M_DrawRoomSprites(obj_ptr + 1, *obj_ptr);
}

void Output_DrawShadow(
    const int16_t size, const BOUNDS_16 *const bounds, const ITEM *const item)
{
    g_ShadowInfo.vertex_count = g_Config.enable_round_shadow ? 32 : 8;

    int32_t x0 = bounds->min.x;
    int32_t x1 = bounds->max.x;
    int32_t z0 = bounds->min.z;
    int32_t z1 = bounds->max.z;

    int32_t x_mid = (x0 + x1) / 2;
    int32_t z_mid = (z0 + z1) / 2;

    int32_t x_add = (x1 - x0) * size / 1024;
    int32_t z_add = (z1 - z0) * size / 1024;

    for (int32_t i = 0; i < g_ShadowInfo.vertex_count; i++) {
        int32_t angle = (PHD_180 + i * PHD_360) / g_ShadowInfo.vertex_count;
        g_ShadowInfo.vertex[i].x =
            x_mid + (x_add * 2) * Math_Sin(angle) / PHD_90;
        g_ShadowInfo.vertex[i].z =
            z_mid + (z_add * 2) * Math_Cos(angle) / PHD_90;
        g_ShadowInfo.vertex[i].y = 0;
    }

    Matrix_Push();
    Matrix_TranslateAbs(
        item->interp.result.pos.x, item->floor, item->interp.result.pos.z);
    Matrix_RotY(item->rot.y);

    if (M_CalcObjectVertices(&g_ShadowInfo.poly_count)) {
        int16_t clip_and = 1;
        int16_t clip_positive = 1;
        int16_t clip_or = 0;
        for (int32_t i = 0; i < g_ShadowInfo.vertex_count; i++) {
            clip_and &= m_VBuf[i].clip;
            clip_positive &= m_VBuf[i].clip >= 0;
            clip_or |= m_VBuf[i].clip;
        }
        PHD_VBUF *vn1 = &m_VBuf[0];
        PHD_VBUF *vn2 = &m_VBuf[g_Config.enable_round_shadow ? 4 : 1];
        PHD_VBUF *vn3 = &m_VBuf[g_Config.enable_round_shadow ? 8 : 2];

        int32_t c1 = (vn3->xs - vn2->xs) * (vn1->ys - vn2->ys);
        int32_t c2 = (vn1->xs - vn2->xs) * (vn3->ys - vn2->ys);
        bool visible = (int32_t)(c1 - c2) >= 0;

        if (!clip_and && clip_positive && visible) {
            S_Output_DrawShadow(
                &m_VBuf[0], clip_or ? 1 : 0, g_ShadowInfo.vertex_count);
        }
    }

    Matrix_Pop();
}

int32_t Output_GetDrawDistMin(void)
{
    return 127;
}

int32_t Output_GetDrawDistFade(void)
{
    return m_DrawDistFade;
}

int32_t Output_GetDrawDistMax(void)
{
    return m_DrawDistMax;
}

void Output_SetDrawDistFade(int32_t dist)
{
    m_DrawDistFade = dist;
}

void Output_SetDrawDistMax(int32_t dist)
{
    m_DrawDistMax = dist;
}

void Output_SetWaterColor(const RGB_F *color)
{
    m_WaterColor.r = color->r;
    m_WaterColor.g = color->g;
    m_WaterColor.b = color->b;
}

int32_t Output_GetNearZ(void)
{
    return Output_GetDrawDistMin() << W2V_SHIFT;
}

int32_t Output_GetFarZ(void)
{
    return Output_GetDrawDistMax() << W2V_SHIFT;
}

void Output_DrawSprite(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade)
{
    x -= g_W2VMatrix._03;
    y -= g_W2VMatrix._13;
    z -= g_W2VMatrix._23;

    if (x < -Output_GetDrawDistMax() || x > Output_GetDrawDistMax()) {
        return;
    }

    if (y < -Output_GetDrawDistMax() || y > Output_GetDrawDistMax()) {
        return;
    }

    if (z < -Output_GetDrawDistMax() || z > Output_GetDrawDistMax()) {
        return;
    }

    int32_t zv =
        g_W2VMatrix._20 * x + g_W2VMatrix._21 * y + g_W2VMatrix._22 * z;
    if (zv < Output_GetNearZ() || zv > Output_GetFarZ()) {
        return;
    }

    int32_t xv =
        g_W2VMatrix._00 * x + g_W2VMatrix._01 * y + g_W2VMatrix._02 * z;
    int32_t yv =
        g_W2VMatrix._10 * x + g_W2VMatrix._11 * y + g_W2VMatrix._12 * z;
    int32_t zp = zv / g_PhdPersp;

    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = Viewport_GetCenterX() + (xv + (sprite->x1 << W2V_SHIFT)) / zp;
    int32_t y1 = Viewport_GetCenterY() + (yv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t x2 = Viewport_GetCenterX() + (xv + (sprite->x2 << W2V_SHIFT)) / zp;
    int32_t y2 = Viewport_GetCenterY() + (yv + (sprite->y2 << W2V_SHIFT)) / zp;
    if (x2 >= Viewport_GetMinX() && y2 >= Viewport_GetMinY()
        && x1 <= Viewport_GetMaxX() && y1 <= Viewport_GetMaxY()) {
        int32_t depth = zv >> W2V_SHIFT;
        shade += M_CalcFogShade(depth);
        CLAMPG(shade, 0x1FFF);
        S_Output_DrawSprite(x1, y1, x2, y2, zv, sprnum, shade);
    }
}

void Output_DrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 color)
{
    S_Output_Draw2DQuad(sx, sy, sx + w, sy + h, color, color, color, color);
}

void Output_DrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br)
{
    S_Output_Draw2DQuad(sx, sy, sx + w, sy + h, tl, tr, bl, br);
}

void Output_DrawScreenLine(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 col)
{
    S_Output_Draw2DLine(sx, sy, sx + w, sy + h, col, col);
}

void Output_DrawScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 colDark,
    RGBA_8888 colLight, int32_t thickness)
{
    float scale = Viewport_GetHeight() / 480.0;
    S_Output_ScreenBox(
        sx - scale, sy - scale, w, h, colDark, colLight,
        thickness * scale / 2.0f);
}

void Output_DrawGradientScreenLine(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 col1,
    RGBA_8888 col2)
{
    S_Output_Draw2DLine(sx, sy, sx + w, sy + h, col1, col2);
}

void Output_DrawGradientScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br, int32_t thickness)
{
    float scale = Viewport_GetHeight() / 480.0;
    S_Output_4ColourTextBox(
        sx - scale, sy - scale, w + scale, h + scale, tl, tr, bl, br,
        thickness * scale / 2.0f);
}

void Output_DrawCentreGradientScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 edge,
    RGBA_8888 center, int32_t thickness)
{
    float scale = Viewport_GetHeight() / 480.0;
    S_Output_2ToneColourTextBox(
        sx - scale, sy - scale, w + scale, h + scale, edge, center,
        thickness * scale / 2.0f);
}

void Output_DrawScreenFBox(int32_t sx, int32_t sy, int32_t w, int32_t h)
{
    RGBA_8888 color = { 0, 0, 0, 128 };
    S_Output_Draw2DQuad(sx, sy, sx + w, sy + h, color, color, color, color);
}

void Output_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int16_t sprnum, int16_t shade, uint16_t flags)
{
    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = sx + (scale_h * (sprite->x1 >> 3) / PHD_ONE);
    int32_t x2 = sx + (scale_h * (sprite->x2 >> 3) / PHD_ONE);
    int32_t y1 = sy + (scale_v * (sprite->y1 >> 3) / PHD_ONE);
    int32_t y2 = sy + (scale_v * (sprite->y2 >> 3) / PHD_ONE);
    if (x2 >= 0 && y2 >= 0 && x1 < Viewport_GetWidth()
        && y1 < Viewport_GetHeight()) {
        S_Output_DrawSprite(x1, y1, x2, y2, 8 * z, sprnum, shade);
    }
}

void Output_DrawScreenSprite2D(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int32_t sprnum, int16_t shade, uint16_t flags, int32_t page)
{
    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = sx + (scale_h * sprite->x1 / PHD_ONE);
    int32_t x2 = sx + (scale_h * sprite->x2 / PHD_ONE);
    int32_t y1 = sy + (scale_v * sprite->y1 / PHD_ONE);
    int32_t y2 = sy + (scale_v * sprite->y2 / PHD_ONE);
    if (x2 >= 0 && y2 >= 0 && x1 < Viewport_GetWidth()
        && y1 < Viewport_GetHeight()) {
        S_Output_DrawSprite(x1, y1, x2, y2, 200, sprnum, 0);
    }
}

void Output_DrawSpriteRel(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade)
{
    int32_t zv = g_MatrixPtr->_20 * x + g_MatrixPtr->_21 * y
        + g_MatrixPtr->_22 * z + g_MatrixPtr->_23;
    if (zv < Output_GetNearZ() || zv > Output_GetFarZ()) {
        return;
    }

    int32_t xv = g_MatrixPtr->_00 * x + g_MatrixPtr->_01 * y
        + g_MatrixPtr->_02 * z + g_MatrixPtr->_03;
    int32_t yv = g_MatrixPtr->_10 * x + g_MatrixPtr->_11 * y
        + g_MatrixPtr->_12 * z + g_MatrixPtr->_13;
    int32_t zp = zv / g_PhdPersp;

    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = Viewport_GetCenterX() + (xv + (sprite->x1 << W2V_SHIFT)) / zp;
    int32_t y1 = Viewport_GetCenterY() + (yv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t x2 = Viewport_GetCenterX() + (xv + (sprite->y1 << W2V_SHIFT)) / zp;
    int32_t y2 = Viewport_GetCenterY() + (yv + (sprite->y2 << W2V_SHIFT)) / zp;
    if (x2 >= Viewport_GetMinX() && y2 >= Viewport_GetMinY()
        && x1 <= Viewport_GetMaxX() && y1 <= Viewport_GetMaxY()) {
        int32_t depth = zv >> W2V_SHIFT;
        shade += M_CalcFogShade(depth);
        CLAMPG(shade, 0x1FFF);
        S_Output_DrawSprite(x1, y1, x2, y2, zv, sprnum, shade);
    }
}

void Output_DrawUISprite(
    int32_t x, int32_t y, int32_t scale, int16_t sprnum, int16_t shade)
{
    PHD_SPRITE *sprite = &g_PhdSpriteInfo[sprnum];
    int32_t x1 = x + (scale * sprite->x1 >> 16);
    int32_t x2 = x + (scale * sprite->x2 >> 16);
    int32_t y1 = y + (scale * sprite->y1 >> 16);
    int32_t y2 = y + (scale * sprite->y2 >> 16);
    if (x2 >= Viewport_GetMinX() && y2 >= Viewport_GetMinY()
        && x1 <= Viewport_GetMaxX() && y1 <= Viewport_GetMaxY()) {
        S_Output_DrawSprite(x1, y1, x2, y2, 200, sprnum, shade);
    }
}

void Output_LoadBackdropImage(const char *filename)
{
    if (!filename) {
        S_Output_DownloadBackdropSurface(NULL);
        Memory_FreePointer(&m_BackdropImagePath);
        return;
    }

    const char *old_path = m_BackdropImagePath;
    m_BackdropImagePath = File_GuessExtension(filename, m_ImageExtensions);
    Memory_FreePointer(&old_path);

    IMAGE *img = Image_CreateFromFileInto(
        m_BackdropImagePath, Viewport_GetWidth(), Viewport_GetHeight(),
        IMAGE_FIT_SMART);
    if (img != NULL) {
        S_Output_DownloadBackdropSurface(img);
        Image_Free(img);
    }
}

void Output_DrawLightningSegment(
    int32_t x1, int32_t y1, const int32_t z1, int32_t x2, int32_t y2,
    const int32_t z2, const int32_t width)
{
    if (m_LightningCount >= MAX_LIGHTNINGS) {
        return;
    }

    if (z1 < Output_GetNearZ() || z1 > Output_GetFarZ()
        || z2 < Output_GetNearZ() || z2 > Output_GetFarZ()) {
        return;
    }

    x1 = Viewport_GetCenterX() + x1 / (z1 / g_PhdPersp);
    y1 = Viewport_GetCenterY() + y1 / (z1 / g_PhdPersp);
    x2 = Viewport_GetCenterX() + x2 / (z2 / g_PhdPersp);
    y2 = Viewport_GetCenterY() + y2 / (z2 / g_PhdPersp);
    const int32_t thickness1 = (width << W2V_SHIFT) / (z1 / g_PhdPersp);
    const int32_t thickness2 = (width << W2V_SHIFT) / (z2 / g_PhdPersp);

    LIGHTNING *const lightning = &m_LightningTable[m_LightningCount];
    lightning->edges[0].pos.x = x1;
    lightning->edges[0].pos.y = y1;
    lightning->edges[0].pos.z = z1;
    lightning->edges[0].thickness = thickness1;
    lightning->edges[1].pos.x = x2;
    lightning->edges[1].pos.y = y2;
    lightning->edges[1].pos.z = z2;
    lightning->edges[1].thickness = thickness2;
    m_LightningCount++;
}

void Output_SetupBelowWater(bool underwater)
{
    m_IsWaterEffect = true;
    m_IsWibbleEffect = !underwater;
    m_IsShadeEffect = true;
}

void Output_SetupAboveWater(bool underwater)
{
    m_IsWaterEffect = false;
    m_IsWibbleEffect = underwater;
    m_IsShadeEffect = underwater;
}

void Output_AnimateFades(void)
{
    if (!g_Config.enable_fade_effects) {
        return;
    }

    double delta = 5.0 * Clock_GetFrameAdvance()
        * Clock_GetElapsedDrawFrames(&m_FadeTimer);
    // make title screen fades faster
    if (Phase_Get() == PHASE_INVENTORY && g_InvMode == INV_TITLE_MODE) {
        delta *= 2.0;
    }

    if (m_OverlayCurAlpha + delta <= m_OverlayDstAlpha) {
        m_OverlayCurAlpha += delta;
    } else if (m_OverlayCurAlpha - delta >= m_OverlayDstAlpha) {
        m_OverlayCurAlpha -= delta;
    } else {
        m_OverlayCurAlpha = m_OverlayDstAlpha;
    }
    if (m_BackdropCurAlpha + delta <= m_BackdropDstAlpha) {
        m_BackdropCurAlpha += delta;
    } else if (m_BackdropCurAlpha - delta >= m_BackdropDstAlpha) {
        m_BackdropCurAlpha -= delta;
    } else {
        m_BackdropCurAlpha = m_BackdropDstAlpha;
    }
}

void Output_AnimateTextures(void)
{
    m_WibbleOffsetDbl +=
        Clock_GetFrameAdvance() * Clock_GetSpeedMultiplier() / 2.0;
    m_WibbleOffset = (int32_t)(m_WibbleOffsetDbl) % WIBBLE_SIZE;

    if (!Clock_CheckElapsedLogicalFrames(&m_AnimatedTexturesTimer, 5)) {
        return;
    }

    const TEXTURE_RANGE *range = g_AnimTextureRanges;
    while (range) {
        int32_t i = 0;
        const PHD_TEXTURE temp = g_PhdTextureInfo[range->textures[i]];
        for (; i < range->num_textures - 1; i++) {
            g_PhdTextureInfo[range->textures[i]] =
                g_PhdTextureInfo[range->textures[i + 1]];
        }
        g_PhdTextureInfo[range->textures[i]] = temp;
        range = range->next_range;
    }

    for (int32_t i = 0; i < STATIC_NUMBER_OF; i++) {
        const STATIC_INFO *const static_info = &g_StaticObjects[i];
        if (!static_info->loaded || static_info->nmeshes >= -1) {
            continue;
        }

        const int32_t num_meshes = -static_info->nmeshes;
        const PHD_SPRITE temp = g_PhdSpriteInfo[static_info->mesh_num];
        for (int32_t j = 0; j < num_meshes - 1; j++) {
            g_PhdSpriteInfo[static_info->mesh_num + j] =
                g_PhdSpriteInfo[static_info->mesh_num + j + 1];
        }
        g_PhdSpriteInfo[static_info->mesh_num + num_meshes - 1] = temp;
    }
}

void Output_RotateLight(int16_t pitch, int16_t yaw)
{
    int32_t cp = Math_Cos(pitch);
    int32_t sp = Math_Sin(pitch);
    int32_t cy = Math_Cos(yaw);
    int32_t sy = Math_Sin(yaw);
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

static void M_DrawBlackOverlay(uint8_t alpha)
{
    int32_t sx = 0;
    int32_t sy = 0;
    int32_t sw = Viewport_GetWidth();
    int32_t sh = Viewport_GetHeight();

    RGBA_8888 background = { 0, 0, 0, alpha };
    S_Output_DisableDepthTest();
    S_Output_ClearDepthBuffer();
    Output_DrawScreenFlatQuad(sx, sy, sw, sh, background);
    S_Output_EnableDepthTest();
}

void Output_DrawBackdropScreen(void)
{
    M_DrawBlackOverlay(m_BackdropCurAlpha);
}

void Output_DrawOverlayScreen(void)
{
    M_DrawBlackOverlay(m_OverlayCurAlpha);
}

void Output_FadeReset(void)
{
    m_BackdropCurAlpha = 0;
    m_OverlayCurAlpha = 0;
    m_BackdropDstAlpha = 0;
    m_OverlayDstAlpha = 0;
    Clock_ResetTimer(&m_FadeTimer);
}

void Output_FadeResetToBlack(void)
{
    m_OverlayCurAlpha = 255;
    m_OverlayDstAlpha = 255;
    Clock_ResetTimer(&m_FadeTimer);
}

void Output_FadeToBlack(bool allow_immediate)
{
    if (g_Config.enable_fade_effects) {
        m_OverlayDstAlpha = 255;
    } else if (allow_immediate) {
        m_OverlayCurAlpha = 255;
    }
}

void Output_FadeToSemiBlack(bool allow_immediate)
{
    if (g_Config.enable_fade_effects) {
        m_BackdropDstAlpha = 128;
        m_OverlayDstAlpha = 0;
    } else if (allow_immediate) {
        m_BackdropCurAlpha = 128;
        m_OverlayCurAlpha = 0;
    }
}

void Output_FadeToTransparent(bool allow_immediate)
{
    if (g_Config.enable_fade_effects) {
        m_BackdropDstAlpha = 0;
        m_OverlayDstAlpha = 0;
    } else if (allow_immediate) {
        m_BackdropCurAlpha = 0;
        m_OverlayCurAlpha = 0;
    }
}

bool Output_FadeIsAnimating(void)
{
    if (!g_Config.enable_fade_effects) {
        return false;
    }
    return m_OverlayCurAlpha != m_OverlayDstAlpha
        || m_BackdropCurAlpha != m_BackdropDstAlpha;
}

void Output_ApplyTint(float *r, float *g, float *b)
{
    if (m_IsShadeEffect) {
        *r *= m_WaterColor.r;
        *g *= m_WaterColor.g;
        *b *= m_WaterColor.b;
    }
}

bool Output_MakeScreenshot(const char *path)
{
    return S_Output_MakeScreenshot(path);
}

int Output_GetObjectBounds(const BOUNDS_16 *const bounds)
{
    if (g_MatrixPtr->_23 >= Output_GetFarZ()) {
        return 0;
    }

    int32_t x_min = bounds->min.x;
    int32_t x_max = bounds->max.x;
    int32_t y_min = bounds->min.y;
    int32_t y_max = bounds->max.y;
    int32_t z_min = bounds->min.z;
    int32_t z_max = bounds->max.z;

    XYZ_32 vtx[8];
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
        int32_t zv = g_MatrixPtr->_20 * vtx[i].x + g_MatrixPtr->_21 * vtx[i].y
            + g_MatrixPtr->_22 * vtx[i].z + g_MatrixPtr->_23;

        if (zv > Output_GetNearZ() && zv < Output_GetFarZ()) {
            ++num_z;
            int32_t zp = zv / g_PhdPersp;
            int32_t xv =
                (g_MatrixPtr->_00 * vtx[i].x + g_MatrixPtr->_01 * vtx[i].y
                 + g_MatrixPtr->_02 * vtx[i].z + g_MatrixPtr->_03)
                / zp;
            int32_t yv =
                (g_MatrixPtr->_10 * vtx[i].x + g_MatrixPtr->_11 * vtx[i].y
                 + g_MatrixPtr->_12 * vtx[i].z + g_MatrixPtr->_13)
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

    x_min += Viewport_GetCenterX();
    x_max += Viewport_GetCenterX();
    y_min += Viewport_GetCenterY();
    y_max += Viewport_GetCenterY();

    if (!num_z || x_min > g_PhdRight || y_min > g_PhdBottom || x_max < g_PhdLeft
        || y_max < g_PhdTop) {
        return 0; // out of screen
    }

    if (num_z < 8 || x_min < 0 || y_min < 0 || x_max > Viewport_GetMaxX()
        || y_max > Viewport_GetMaxY()) {
        return -1; // clipped
    }

    return 1; // fully on screen
}

void Output_SetPalette(const RGB_888 *palette, const size_t palette_size)
{
    m_ColorPalette =
        Memory_Realloc(m_ColorPalette, sizeof(RGB_888) * palette_size);
    memcpy(m_ColorPalette, palette, sizeof(RGB_888) * palette_size);
}

RGB_888 Output_GetPaletteColor(uint16_t idx)
{
    if (m_ColorPalette == NULL) {
        return (RGB_888) { 0 };
    }
    return m_ColorPalette[idx];
}
