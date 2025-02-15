#include "game/output.h"

#include "game/clock.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/room.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/s_output.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/engine/image.h>
#include <libtrx/filesystem.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/gfx/context.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>

#include <math.h>
#include <string.h>

#define MAX_LIGHTNINGS 64
#define PHD_IONE (PHD_ONE / 4)

typedef struct {
    struct {
        XYZ_32 pos;
        int32_t thickness;
    } edges[2];
} LIGHTNING;

typedef struct {
    int16_t poly_count;
    int16_t vertex_count;
    XYZ_16 vertices[32];
} SHADOW_INFO;

static int32_t m_LsAdder = 0;
static int32_t m_LsDivider = 0;
static bool m_IsSkyboxEnabled = false;
static bool m_IsWibbleEffect = false;
static bool m_IsWaterEffect = false;
static bool m_IsShadeEffect = false;
static int m_OverlayCurAlpha = 0;
static int m_OverlayDstAlpha = 0;
static int m_BackdropCurAlpha = 0;
static int m_BackdropDstAlpha = 0;

static int32_t m_WibbleOffset = 0;
static int32_t m_AnimatedTexturesOffset = 0;
static CLOCK_TIMER m_WibbleTimer = { .type = CLOCK_TIMER_SIM };
static CLOCK_TIMER m_AnimatedTexturesTimer = { .type = CLOCK_TIMER_SIM };
static CLOCK_TIMER m_FadeTimer = { .type = CLOCK_TIMER_SIM };
static int32_t m_WibbleTable[WIBBLE_SIZE] = {};
static int32_t m_ShadeTable[WIBBLE_SIZE] = {};
static int32_t m_RandTable[WIBBLE_SIZE] = {};

static PHD_VBUF *m_VBuf = nullptr;
static TEXTURE_UV *m_EnvMapUV = nullptr;
static int32_t m_DrawDistFade = 0;
static int32_t m_DrawDistMax = 0;
static RGB_F m_WaterColor = {};
static XYZ_32 m_LsVectorView = {};

static int32_t m_LightningCount = 0;
static LIGHTNING m_LightningTable[MAX_LIGHTNINGS];

static char *m_BackdropImagePath = nullptr;
static const char *m_ImageExtensions[] = {
    ".png", ".jpg", ".jpeg", ".pcx", nullptr,
};

static void M_DrawFlatFace3s(const FACE3 *faces, int32_t count);
static void M_DrawFlatFace4s(const FACE4 *faces, int32_t count);
static void M_DrawTexturedFace3s(const FACE3 *faces, int32_t count);
static void M_DrawTexturedFace4s(const FACE4 *faces, int32_t count);
static void M_DrawObjectFace3EnvMap(const FACE3 *faces, int32_t count);
static void M_DrawObjectFace4EnvMap(const FACE4 *faces, int32_t count);
static void M_DrawRoomSprites(const ROOM_MESH *mesh);
static uint16_t M_CalcVertex(PHD_VBUF *vbuf, const XYZ_16 pos);
static void M_CalcVertexWibble(PHD_VBUF *vbuf);
static bool M_CalcObjectVertices(const XYZ_16 *vertices, int16_t count);
static void M_CalcVerticeLight(const OBJECT_MESH *mesh);
static bool M_CalcVerticeEnvMap(const OBJECT_MESH *mesh);
static void M_CalcSkyboxLight(const OBJECT_MESH *mesh);
static void M_CalcRoomVertices(const ROOM_MESH *mesh);
static void M_CalcRoomVerticesWibble(const ROOM_MESH *mesh);
static void M_CalcWibbleTable(void);

static void M_DrawFlatFace3s(const FACE3 *const faces, const int32_t count)
{
    S_Output_DisableTextureMode();

    for (int32_t i = 0; i < count; i++) {
        const FACE3 *const face = &faces[i];
        PHD_VBUF *const vns[3] = {
            &m_VBuf[face->vertices[0]],
            &m_VBuf[face->vertices[1]],
            &m_VBuf[face->vertices[2]],
        };

        const RGBA_8888 color =
            Output_RGB2RGBA(Output_GetPaletteColor8(face->palette_idx));
        S_Output_DrawFlatTriangle(vns[0], vns[1], vns[2], color);
    }
}

static void M_DrawFlatFace4s(const FACE4 *const faces, const int32_t count)
{
    S_Output_DisableTextureMode();

    for (int32_t i = 0; i < count; i++) {
        const FACE4 *const face = &faces[i];
        PHD_VBUF *const vns[4] = {
            &m_VBuf[face->vertices[0]],
            &m_VBuf[face->vertices[1]],
            &m_VBuf[face->vertices[2]],
            &m_VBuf[face->vertices[3]],
        };

        const RGBA_8888 color =
            Output_RGB2RGBA(Output_GetPaletteColor8(face->palette_idx));
        S_Output_DrawFlatTriangle(vns[0], vns[1], vns[2], color);
        S_Output_DrawFlatTriangle(vns[2], vns[3], vns[0], color);
    }
}

static void M_DrawTexturedFace3s(const FACE3 *const faces, const int32_t count)
{
    S_Output_EnableTextureMode();

    for (int32_t i = 0; i < count; i++) {
        const FACE3 *const face = &faces[i];
        PHD_VBUF *const vns[3] = {
            &m_VBuf[face->vertices[0]],
            &m_VBuf[face->vertices[1]],
            &m_VBuf[face->vertices[2]],
        };

        OBJECT_TEXTURE *const tex = Output_GetObjectTexture(face->texture_idx);
        for (int32_t j = 0; j < 3; j++) {
            vns[j]->u = tex->uv[j].u;
            vns[j]->v = tex->uv[j].v;
        }

        S_Output_DrawTexturedTriangle(
            vns[0], vns[1], vns[2], tex->tex_page, tex->draw_type);
    }
}

static void M_DrawTexturedFace4s(const FACE4 *const faces, const int32_t count)
{
    S_Output_EnableTextureMode();

    for (int32_t i = 0; i < count; i++) {
        const FACE4 *const face = &faces[i];
        PHD_VBUF *const vns[4] = {
            &m_VBuf[face->vertices[0]],
            &m_VBuf[face->vertices[1]],
            &m_VBuf[face->vertices[2]],
            &m_VBuf[face->vertices[3]],
        };

        OBJECT_TEXTURE *const tex = Output_GetObjectTexture(face->texture_idx);
        for (int32_t j = 0; j < 4; j++) {
            vns[j]->u = tex->uv[j].u;
            vns[j]->v = tex->uv[j].v;
        }

        S_Output_DrawTexturedQuad(
            vns[0], vns[1], vns[2], vns[3], tex->tex_page, tex->draw_type);
    }
}

static void M_DrawObjectFace3EnvMap(
    const FACE3 *const faces, const int32_t count)
{
    for (int32_t i = 0; i < count; i++) {
        const FACE3 *const face = &faces[i];
        PHD_VBUF *vns[3] = {
            &m_VBuf[face->vertices[0]],
            &m_VBuf[face->vertices[1]],
            &m_VBuf[face->vertices[2]],
        };

        for (int32_t j = 0; j < 3; j++) {
            vns[j]->u = m_EnvMapUV[face->vertices[j]].u;
            vns[j]->v = m_EnvMapUV[face->vertices[j]].v;
        }

        if (face->enable_reflections) {
            S_Output_DrawEnvMapTriangle(vns[0], vns[1], vns[2]);
        }
    }
}

static void M_DrawObjectFace4EnvMap(
    const FACE4 *const faces, const int32_t count)
{
    for (int32_t i = 0; i < count; i++) {
        const FACE4 *const face = &faces[i];
        PHD_VBUF *vns[4] = {
            &m_VBuf[face->vertices[0]],
            &m_VBuf[face->vertices[1]],
            &m_VBuf[face->vertices[2]],
            &m_VBuf[face->vertices[3]],
        };

        for (int32_t j = 0; j < 4; j++) {
            vns[j]->u = m_EnvMapUV[face->vertices[j]].u;
            vns[j]->v = m_EnvMapUV[face->vertices[j]].v;
        }

        if (face->enable_reflections) {
            S_Output_DrawEnvMapQuad(vns[0], vns[1], vns[2], vns[3]);
        }
    }
}

static void M_DrawRoomSprites(const ROOM_MESH *const mesh)
{
    for (int i = 0; i < mesh->num_sprites; i++) {
        const ROOM_SPRITE *room_sprite = &mesh->sprites[i];
        const PHD_VBUF *const vbuf = &m_VBuf[room_sprite->vertex];
        if (vbuf->clip < 0) {
            continue;
        }

        const int32_t zv = vbuf->zv;
        const SPRITE_TEXTURE *const sprite =
            Output_GetSpriteTexture(room_sprite->texture);
        const int32_t zp = (zv / g_PhdPersp);
        const int32_t x0 =
            Viewport_GetCenterX() + (vbuf->xv + (sprite->x0 << W2V_SHIFT)) / zp;
        const int32_t y0 =
            Viewport_GetCenterY() + (vbuf->yv + (sprite->y0 << W2V_SHIFT)) / zp;
        const int32_t x1 =
            Viewport_GetCenterX() + (vbuf->xv + (sprite->x1 << W2V_SHIFT)) / zp;
        const int32_t y1 =
            Viewport_GetCenterY() + (vbuf->yv + (sprite->y1 << W2V_SHIFT)) / zp;
        if (x1 >= g_PhdLeft && y1 >= g_PhdTop && x0 < g_PhdRight
            && y0 < g_PhdBottom) {
            S_Output_DrawSprite(
                x0, y0, x1, y1, zv, room_sprite->texture, vbuf->g);
        }
    }
}

static uint16_t M_CalcVertex(PHD_VBUF *const vbuf, const XYZ_16 pos)
{
    // clang-format off
    double xv =
        g_MatrixPtr->_00 * pos.x +
        g_MatrixPtr->_01 * pos.y +
        g_MatrixPtr->_02 * pos.z +
        g_MatrixPtr->_03;
    double yv =
        g_MatrixPtr->_10 * pos.x +
        g_MatrixPtr->_11 * pos.y +
        g_MatrixPtr->_12 * pos.z +
        g_MatrixPtr->_13;
    double zv =
        g_MatrixPtr->_20 * pos.x +
        g_MatrixPtr->_21 * pos.y +
        g_MatrixPtr->_22 * pos.z +
        g_MatrixPtr->_23;
    // clang-format on

    vbuf->xv = xv;
    vbuf->yv = yv;
    vbuf->zv = zv;

    uint16_t clip_flags;
    if (zv < Output_GetNearZ()) {
        clip_flags = 0x8000;
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

        vbuf->xs = xs;
        vbuf->ys = ys;
    }
    vbuf->clip = clip_flags;
    return clip_flags;
}

static void M_CalcVertexWibble(PHD_VBUF *const vbuf)
{
    double xs = vbuf->xs;
    double ys = vbuf->ys;
    xs += m_WibbleTable[(m_WibbleOffset + (int)ys) & (WIBBLE_SIZE - 1)];
    ys += m_WibbleTable[(m_WibbleOffset + (int)xs) & (WIBBLE_SIZE - 1)];

    int16_t clip_flags = vbuf->clip & ~15;
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

    vbuf->xs = xs;
    vbuf->ys = ys;
    vbuf->clip = clip_flags;
}

static bool M_CalcObjectVertices(
    const XYZ_16 *const vertices, const int16_t count)
{
    uint16_t total_clip = 0xFFFF;
    for (int i = 0; i < count; i++) {
        total_clip &= M_CalcVertex(&m_VBuf[i], vertices[i]);
    }

    return total_clip == 0;
}

static void M_CalcVerticeLight(const OBJECT_MESH *const mesh)
{
    if (mesh->num_lights <= 0) {
        for (int32_t i = 0; i < -mesh->num_lights; i++) {
            int16_t shade = m_LsAdder + mesh->lighting.lights[i];
            CLAMP(shade, 0, 0x1FFF);
            m_VBuf[i].g = shade;
        }

        return;
    }

    if (m_LsDivider == 0) {
        int16_t shade = m_LsAdder;
        CLAMP(shade, 0, 0x1FFF);
        for (int32_t i = 0; i < mesh->num_lights; i++) {
            m_VBuf[i].g = shade;
        }

        return;
    }

    // clang-format off
    const int32_t xv = (
        g_MatrixPtr->_00 * m_LsVectorView.x +
        g_MatrixPtr->_10 * m_LsVectorView.y +
        g_MatrixPtr->_20 * m_LsVectorView.z
    ) / m_LsDivider;

    const int32_t yv = (
        g_MatrixPtr->_01 * m_LsVectorView.x +
        g_MatrixPtr->_11 * m_LsVectorView.y +
        g_MatrixPtr->_21 * m_LsVectorView.z
    ) / m_LsDivider;

    const int32_t zv = (
        g_MatrixPtr->_02 * m_LsVectorView.x +
        g_MatrixPtr->_12 * m_LsVectorView.y +
        g_MatrixPtr->_22 * m_LsVectorView.z
    ) / m_LsDivider;
    // clang-format on

    for (int32_t i = 0; i < mesh->num_lights; i++) {
        const XYZ_16 *const normal = &mesh->lighting.normals[i];
        int16_t shade = m_LsAdder
            + ((normal->x * xv + normal->y * yv + normal->z * zv) >> 16);
        CLAMP(shade, 0, 0x1FFF);
        m_VBuf[i].g = shade;
    }
}

static bool M_CalcVerticeEnvMap(const OBJECT_MESH *mesh)
{
    if (mesh->num_lights <= 0) {
        return false;
    }

    for (int32_t i = 0; i < mesh->num_lights; ++i) {
        // make sure that reflection will be drawn after normal poly
        m_VBuf[i].zv -= (double)((1 << W2V_SHIFT) / 2);

        // set lighting that depends only from fog distance
        m_VBuf[i].g = 0x1000;

        const int32_t depth = g_MatrixPtr->_23 >> W2V_SHIFT;
        m_VBuf[i].g += Output_CalcFogShade(depth);

        // reflection can be darker but not brighter
        CLAMP(m_VBuf[i].g, 0x1000, 0x1FFF);

        // rotate normal vectors for X/Y, no translation
        const XYZ_16 *const normal = &mesh->lighting.normals[i];

        // clang-format off
        int32_t x = (
            g_MatrixPtr->_00 * normal->x +
            g_MatrixPtr->_01 * normal->y +
            g_MatrixPtr->_02 * normal->z
        ) >> W2V_SHIFT;

        int32_t y = (
            g_MatrixPtr->_10 * normal->x +
            g_MatrixPtr->_11 * normal->y +
            g_MatrixPtr->_12 * normal->z
        ) >> W2V_SHIFT;
        // clang-format on

        CLAMP(x, -PHD_ONE, PHD_IONE);
        CLAMP(y, -PHD_ONE, PHD_IONE);
        m_EnvMapUV[i].u = PHD_ONE / PHD_IONE * (x + PHD_IONE) / 2;
        m_EnvMapUV[i].v = PHD_ONE - PHD_ONE / PHD_IONE * (y + PHD_IONE) / 2;
    }

    return true;
}

static void M_CalcSkyboxLight(const OBJECT_MESH *const mesh)
{
    for (int32_t i = 0; i < ABS(mesh->num_lights); i++) {
        m_VBuf[i].g = 0xFFF;
    }
}

static void M_CalcRoomVertices(const ROOM_MESH *const mesh)
{
    for (int32_t i = 0; i < mesh->num_vertices; i++) {
        PHD_VBUF *const vbuf = &m_VBuf[i];
        const ROOM_VERTEX *const vertex = &mesh->vertices[i];

        M_CalcVertex(vbuf, vertex->pos);

        vbuf->g = vertex->light_adder;
        if (vbuf->zv >= Output_GetNearZ()) {
            const int32_t depth = ((int32_t)vbuf->zv) >> W2V_SHIFT;
            if (depth > Output_GetDrawDistMax()) {
                vbuf->g = MAX_LIGHTING;
                if (!m_IsSkyboxEnabled) {
                    vbuf->clip |= 16;
                }
            } else {
                vbuf->g += Output_CalcFogShade(depth);
                if (!m_IsWaterEffect) {
                    CLAMPG(vbuf->g, MAX_LIGHTING);
                }
            }
        }

        if (m_IsWaterEffect) {
            vbuf->g += m_ShadeTable[(
                ((uint8_t)m_WibbleOffset
                 + (uint8_t)m_RandTable[(mesh->num_vertices - i) % WIBBLE_SIZE])
                % WIBBLE_SIZE)];
            CLAMP(vbuf->g, 0, 0x1FFF);
        }
    }
}

static void M_CalcRoomVerticesWibble(const ROOM_MESH *const mesh)
{
    for (int32_t i = 0; i < mesh->num_vertices; i++) {
        if (mesh->vertices[i].flags & NO_VERT_MOVE) {
            continue;
        }
        M_CalcVertexWibble(&m_VBuf[i]);
    }
}

static void M_CalcWibbleTable(void)
{
    for (int i = 0; i < WIBBLE_SIZE; i++) {
        PHD_ANGLE angle = (i * DEG_360) / WIBBLE_SIZE;
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
}

void Output_ReserveVertexBuffer(const size_t size)
{
    m_VBuf = GameBuf_Alloc(size * sizeof(PHD_VBUF), GBUF_VERTEX_BUFFER);
    m_EnvMapUV = GameBuf_Alloc(size * sizeof(TEXTURE_UV), GBUF_VERTEX_BUFFER);
}

void Output_SetWindowSize(int width, int height)
{
    S_Output_SetWindowSize(width, height);
}

void Output_ApplyRenderSettings(void)
{
    S_Output_ApplyRenderSettings();
    if (m_BackdropImagePath) {
        Output_LoadBackgroundFromFile(m_BackdropImagePath);
    }
}

void Output_DownloadTextures(int page_count)
{
    S_Output_DownloadTextures(page_count);
}

void Output_DrawBlack(void)
{
    Output_DrawBlackRectangle(255);
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
    Output_ApplyFOV();
    Text_DrawReset();

    S_Output_RenderBegin();
    m_LightningCount = 0;
}

void Output_EndScene(void)
{
    S_Output_DisableDepthTest();
    S_Output_ClearDepthBuffer();
    Overlay_DrawFPSInfo();
    S_Output_EnableDepthTest();
    S_Output_RenderEnd();
    S_Output_FlipScreen();
    Shell_ProcessEvents();
    g_FPSCounter++;
}

void Output_ClearDepthBuffer(void)
{
    S_Output_ClearDepthBuffer();
}

void Output_DrawObjectMesh(const OBJECT_MESH *const mesh, const int32_t clip)
{
    if (!M_CalcObjectVertices(mesh->vertices, mesh->num_vertices)) {
        return;
    }

    M_CalcVerticeLight(mesh);
    M_DrawTexturedFace4s(mesh->tex_face4s, mesh->num_tex_face4s);
    M_DrawTexturedFace3s(mesh->tex_face3s, mesh->num_tex_face3s);
    M_DrawFlatFace4s(mesh->flat_face4s, mesh->num_flat_face4s);
    M_DrawFlatFace3s(mesh->flat_face3s, mesh->num_flat_face3s);

    if (mesh->enable_reflections && g_Config.visuals.enable_reflections) {
        if (!M_CalcVerticeEnvMap(mesh)) {
            return;
        }

        M_DrawObjectFace4EnvMap(mesh->tex_face4s, mesh->num_tex_face4s);
        M_DrawObjectFace3EnvMap(mesh->tex_face3s, mesh->num_tex_face3s);
        M_DrawObjectFace4EnvMap(mesh->flat_face4s, mesh->num_flat_face4s);
        M_DrawObjectFace3EnvMap(mesh->flat_face3s, mesh->num_flat_face3s);
    }
}

void Output_DrawObjectMesh_I(const OBJECT_MESH *const mesh, const int32_t clip)
{
    Matrix_Push();
    Matrix_Interpolate();
    Output_DrawObjectMesh(mesh, clip);
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

void Output_DrawSkybox(const OBJECT_MESH *const mesh)
{
    g_PhdLeft = Viewport_GetMinX();
    g_PhdTop = Viewport_GetMinY();
    g_PhdRight = Viewport_GetMaxX();
    g_PhdBottom = Viewport_GetMaxY();

    if (!M_CalcObjectVertices(mesh->vertices, mesh->num_vertices)) {
        return;
    }

    S_Output_DisableDepthTest();
    M_CalcSkyboxLight(mesh);
    M_DrawTexturedFace4s(mesh->tex_face4s, mesh->num_tex_face4s);
    M_DrawTexturedFace3s(mesh->tex_face3s, mesh->num_tex_face3s);
    M_DrawFlatFace4s(mesh->flat_face4s, mesh->num_flat_face4s);
    M_DrawFlatFace3s(mesh->flat_face3s, mesh->num_flat_face3s);
    S_Output_EnableDepthTest();
}

void Output_DrawRoom(const ROOM_MESH *const mesh)
{
    M_CalcRoomVertices(mesh);

    if (m_IsWibbleEffect) {
        S_Output_DisableDepthWrites();
        M_DrawTexturedFace4s(mesh->face4s, mesh->num_face4s);
        M_DrawTexturedFace3s(mesh->face3s, mesh->num_face3s);
        S_Output_EnableDepthWrites();
        M_CalcRoomVerticesWibble(mesh);
    }

    M_DrawTexturedFace4s(mesh->face4s, mesh->num_face4s);
    M_DrawTexturedFace3s(mesh->face3s, mesh->num_face3s);
    M_DrawRoomSprites(mesh);
}

void Output_DrawRoomPortals(const ROOM *const room)
{
    S_Output_DisableDepthTest();
    const RGBA_8888 portal_color = { 0, 0, 255, 255 };
    for (int32_t i = 0; i < room->portals->count; i++) {
        const PORTAL *const portal = &room->portals->portal[i];
        const XYZ_32 vertices[4] = {
            { portal->vertex[0].x, portal->vertex[0].y, portal->vertex[0].z },
            { portal->vertex[1].x, portal->vertex[1].y, portal->vertex[1].z },
            { portal->vertex[2].x, portal->vertex[2].y, portal->vertex[2].z },
            { portal->vertex[3].x, portal->vertex[3].y, portal->vertex[3].z },
        };
        Output_Draw3DFrame(vertices, portal_color);
    }
    S_Output_EnableDepthTest();
}

void Output_DrawRoomTriggers(const ROOM *const room)
{
#define DRAW_TRI(a, b, c, color)                                               \
    do {                                                                       \
        S_Output_DrawFlatTriangle(a, b, c, color);                             \
        S_Output_DrawFlatTriangle(c, b, a, color);                             \
    } while (0)
#define DRAW_QUAD(a, b, c, d, color)                                           \
    do {                                                                       \
        DRAW_TRI(a, b, d, color);                                              \
        DRAW_TRI(b, c, d, color);                                              \
    } while (0)

    const RGBA_8888 color = { .r = 255, .g = 0, .b = 255, .a = 128 };
    const XZ_16 offsets[4] = { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };

    m_IsWaterEffect = false;
    m_IsShadeEffect = false;
    S_Output_DisableTextureMode();
    S_Output_DisableDepthWrites();
    S_Output_SetBlendingMode(GFX_BLEND_MODE_NORMAL);
    for (int32_t z = 0; z < room->size.z; z++) {
        for (int32_t x = 0; x < room->size.x; x++) {
            const SECTOR *sector = Room_GetUnitSector(room, x, z);
            if (sector->trigger == nullptr) {
                continue;
            }
            PHD_VBUF vns[4];
            for (int32_t i = 0; i < 4; i++) {
                XYZ_16 vertex_pos = {
                    .x = (x + offsets[i].x) * WALL_L,
                    .z = (z + offsets[i].z) * WALL_L,
                };
                XYZ_32 world_pos = {
                    .x = room->pos.x + x * WALL_L + offsets[i].x * (WALL_L - 1),
                    .z = room->pos.z + z * WALL_L + offsets[i].z * (WALL_L - 1),
                    .y = room->pos.y,
                };

                int16_t room_num = room - Room_Get(0);
                sector = Room_GetSector(
                    world_pos.x, world_pos.y, world_pos.z, &room_num);
                vertex_pos.y =
                    Room_GetHeight(
                        sector, world_pos.x, world_pos.y, world_pos.z)
                    + (m_IsWaterEffect ? -16 : -2);

                M_CalcVertex(&vns[i], vertex_pos);
                vns[i].g = HIGH_LIGHT;
                vns[i].zv -=
                    (double)((1 << W2V_SHIFT) / 2); // reduce z fighting
            }

            DRAW_QUAD(&vns[0], &vns[1], &vns[2], &vns[3], color);
        }
    }

    S_Output_SetBlendingMode(GFX_BLEND_MODE_OFF);
    S_Output_EnableDepthWrites();

#undef DRAW_TRI
#undef DRAW_QUAD
}

void Output_DrawShadow(
    const int16_t size, const BOUNDS_16 *const bounds, const ITEM *const item)
{
    if (!item->enable_shadow) {
        return;
    }

    SHADOW_INFO shadow = {};
    shadow.vertex_count = g_Config.visuals.enable_round_shadow ? 32 : 8;

    int32_t x0 = bounds->min.x;
    int32_t x1 = bounds->max.x;
    int32_t z0 = bounds->min.z;
    int32_t z1 = bounds->max.z;

    int32_t x_mid = (x0 + x1) / 2;
    int32_t z_mid = (z0 + z1) / 2;

    int32_t x_add = (x1 - x0) * size / 1024;
    int32_t z_add = (z1 - z0) * size / 1024;

    for (int32_t i = 0; i < shadow.vertex_count; i++) {
        int32_t angle = (DEG_180 + i * DEG_360) / shadow.vertex_count;
        shadow.vertices[i].x = x_mid + (x_add * 2) * Math_Sin(angle) / DEG_90;
        shadow.vertices[i].z = z_mid + (z_add * 2) * Math_Cos(angle) / DEG_90;
        shadow.vertices[i].y = 0;
    }

    Matrix_Push();
    Matrix_TranslateAbs(
        item->interp.result.pos.x, item->floor, item->interp.result.pos.z);
    Matrix_RotY(item->rot.y);

    if (M_CalcObjectVertices(shadow.vertices, shadow.vertex_count)) {
        int16_t clip_and = 1;
        int16_t clip_positive = 1;
        int16_t clip_or = 0;
        for (int32_t i = 0; i < shadow.vertex_count; i++) {
            clip_and &= m_VBuf[i].clip;
            clip_positive &= m_VBuf[i].clip >= 0;
            clip_or |= m_VBuf[i].clip;
        }
        PHD_VBUF *vn1 = &m_VBuf[0];
        PHD_VBUF *vn2 = &m_VBuf[g_Config.visuals.enable_round_shadow ? 4 : 1];
        PHD_VBUF *vn3 = &m_VBuf[g_Config.visuals.enable_round_shadow ? 8 : 2];

        int32_t c1 = (vn3->xs - vn2->xs) * (vn1->ys - vn2->ys);
        int32_t c2 = (vn1->xs - vn2->xs) * (vn3->ys - vn2->ys);
        bool visible = (int32_t)(c1 - c2) >= 0;

        if (!clip_and && clip_positive && visible) {
            S_Output_DrawShadow(
                &m_VBuf[0], clip_or ? 1 : 0, shadow.vertex_count);
        }
    }

    Matrix_Pop();
}

int32_t Output_GetDrawDistMin(void)
{
    return 20;
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

    const double near_z = Output_GetNearZ();
    const double far_z = Output_GetFarZ();
    const double res_z = 0.99 * near_z * far_z / (far_z - near_z);
    g_FltResZ = res_z;
    g_FltResZBuf = 0.005 + res_z / near_z;
}

void Output_SetWaterColor(const RGB_F *color)
{
    m_WaterColor.r = color->r;
    m_WaterColor.g = color->g;
    m_WaterColor.b = color->b;
}

void Output_SetLightAdder(const int32_t adder)
{
    m_LsAdder = adder;
}

void Output_SetLightDivider(const int32_t divider)
{
    m_LsDivider = divider;
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

    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprnum);
    const int32_t x0 =
        Viewport_GetCenterX() + (xv + (sprite->x0 << W2V_SHIFT)) / zp;
    const int32_t y0 =
        Viewport_GetCenterY() + (yv + (sprite->y0 << W2V_SHIFT)) / zp;
    const int32_t x1 =
        Viewport_GetCenterX() + (xv + (sprite->x1 << W2V_SHIFT)) / zp;
    const int32_t y1 =
        Viewport_GetCenterY() + (yv + (sprite->y1 << W2V_SHIFT)) / zp;
    if (x1 >= Viewport_GetMinX() && y1 >= Viewport_GetMinY()
        && x0 <= Viewport_GetMaxX() && y0 <= Viewport_GetMaxY()) {
        int32_t depth = zv >> W2V_SHIFT;
        shade += Output_CalcFogShade(depth);
        CLAMPG(shade, 0x1FFF);
        S_Output_DrawSprite(x0, y0, x1, y1, zv, sprnum, shade);
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

void Output_Draw3DLine(
    const XYZ_32 pos_0, const XYZ_32 pos_1, const RGBA_8888 color)
{
    PHD_VBUF vbuf[2];
    uint16_t total_clip = 0xFFFF;
    total_clip &= M_CalcVertex(
        &vbuf[0], (XYZ_16) { .x = pos_0.x, .y = pos_0.y, .z = pos_0.z });
    total_clip &= M_CalcVertex(
        &vbuf[1], (XYZ_16) { .x = pos_1.x, .y = pos_1.y, .z = pos_1.z });
    S_Output_Draw3DLine(&vbuf[0], &vbuf[1], color);
}

void Output_Draw3DFrame(const XYZ_32 vert[4], const RGBA_8888 color)
{
    Output_Draw3DLine(vert[0], vert[1], color);
    Output_Draw3DLine(vert[1], vert[2], color);
    Output_Draw3DLine(vert[2], vert[3], color);
    Output_Draw3DLine(vert[3], vert[0], color);
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

void Output_DrawScreenSprite2D(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int32_t sprnum, int16_t shade, uint16_t flags, int32_t page)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprnum);
    const int32_t x0 = sx + (scale_h * sprite->x0 / PHD_ONE);
    const int32_t x1 = sx + (scale_h * sprite->x1 / PHD_ONE);
    const int32_t y0 = sy + (scale_v * sprite->y0 / PHD_ONE);
    const int32_t y1 = sy + (scale_v * sprite->y1 / PHD_ONE);
    if (x1 >= 0 && y1 >= 0 && x0 < Viewport_GetWidth()
        && y0 < Viewport_GetHeight()) {
        S_Output_DrawSprite(x0, y0, x1, y1, Output_GetNearZ() + 200, sprnum, 0);
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

    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprnum);
    const int32_t x0 =
        Viewport_GetCenterX() + (xv + (sprite->x0 << W2V_SHIFT)) / zp;
    const int32_t y0 =
        Viewport_GetCenterY() + (yv + (sprite->y0 << W2V_SHIFT)) / zp;
    const int32_t x1 =
        Viewport_GetCenterX() + (xv + (sprite->y0 << W2V_SHIFT)) / zp;
    const int32_t y1 =
        Viewport_GetCenterY() + (yv + (sprite->y1 << W2V_SHIFT)) / zp;
    if (x1 >= Viewport_GetMinX() && y1 >= Viewport_GetMinY()
        && x0 <= Viewport_GetMaxX() && y0 <= Viewport_GetMaxY()) {
        int32_t depth = zv >> W2V_SHIFT;
        shade += Output_CalcFogShade(depth);
        CLAMPG(shade, 0x1FFF);
        S_Output_DrawSprite(x0, y0, x1, y1, zv, sprnum, shade);
    }
}

void Output_DrawUISprite(
    int32_t x, int32_t y, int32_t scale, int16_t sprnum, int16_t shade)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprnum);
    const int32_t x0 = x + (scale * sprite->x0 >> 16);
    const int32_t x1 = x + (scale * sprite->x1 >> 16);
    const int32_t y0 = y + (scale * sprite->y0 >> 16);
    const int32_t y1 = y + (scale * sprite->y1 >> 16);
    if (x1 >= Viewport_GetMinX() && y1 >= Viewport_GetMinY()
        && x0 <= Viewport_GetMaxX() && y0 <= Viewport_GetMaxY()) {
        S_Output_DrawSprite(
            x0, y0, x1, y1, Output_GetNearZ() + 200, sprnum, shade);
    }
}

bool Output_LoadBackgroundFromFile(const char *const path)
{
    ASSERT(path != nullptr);
    const char *old_path = m_BackdropImagePath;
    m_BackdropImagePath = File_GuessExtension(path, m_ImageExtensions);
    Memory_FreePointer(&old_path);

    IMAGE *const img = Image_CreateFromFileInto(
        m_BackdropImagePath, Viewport_GetWidth(), Viewport_GetHeight(),
        IMAGE_FIT_SMART);
    if (img == nullptr) {
        return false;
    }
    S_Output_DownloadBackdropSurface(img);
    Image_Free(img);
    return true;
}

void Output_LoadBackgroundFromObject(void)
{
    // TR1 doesn't have inventory background object.
    Output_UnloadBackground();
}

void Output_UnloadBackground(void)
{
    S_Output_DownloadBackdropSurface(nullptr);
    Memory_FreePointer(&m_BackdropImagePath);
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

void Output_AnimateTextures(const int32_t num_frames)
{
    m_WibbleOffset = (m_WibbleOffset + num_frames) % WIBBLE_SIZE;
    m_AnimatedTexturesOffset += num_frames;
    while (m_AnimatedTexturesOffset > 5) {
        Output_CycleAnimatedTextures();
        m_AnimatedTexturesOffset -= 5;
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

void Output_DrawBlackRectangle(int32_t opacity)
{
    int32_t sx = 0;
    int32_t sy = 0;
    int32_t sw = Viewport_GetWidth();
    int32_t sh = Viewport_GetHeight();

    RGBA_8888 background = { 0, 0, 0, opacity };
    S_Output_DisableDepthTest();
    S_Output_ClearDepthBuffer();
    Output_DrawScreenFlatQuad(sx, sy, sw, sh, background);
    S_Output_EnableDepthTest();
}

void Output_DrawBackground(void)
{
    // already handled in S_Output_RenderBegin
}

void Output_DrawPolyList(void)
{
    // force flush the vertex stream
    S_Output_ClearDepthBuffer();
}

void Output_ApplyFOV(void)
{
    int32_t fov = Viewport_GetFOV();

    // In places that use GAME_FOV, it can be safely changed to user's choice.
    // But for cinematics, the FOV value chosen by devs needs to stay
    // unchanged, otherwise the game renders the low camera in the Lost Valley
    // cutscene wrong.
    if (g_Config.visuals.fov_vertical) {
        double aspect_ratio =
            Screen_GetResWidth() / (double)Screen_GetResHeight();
        double fov_rad_h = fov * M_PI / (180 * DEG_1);
        double fov_rad_v = 2 * atan(aspect_ratio * tan(fov_rad_h / 2));
        fov = round((fov_rad_v / M_PI) * (180 * DEG_1));
    }

    int16_t c = Math_Cos(fov / 2);
    int16_t s = Math_Sin(fov / 2);

    g_PhdPersp = Screen_GetResWidth() / 2;
    if (s != 0) {
        g_PhdPersp *= c;
        g_PhdPersp /= s;
    }
}

void Output_ApplyTint(float *r, float *g, float *b)
{
    if (m_IsShadeEffect) {
        *r *= m_WaterColor.r;
        *g *= m_WaterColor.g;
        *b *= m_WaterColor.b;
    }
}

bool Output_MakeScreenshot(const char *const path)
{
    GFX_Context_ScheduleScreenshot(path);
    return true;
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

int32_t Output_CalcFogShade(const int32_t depth)
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

int32_t Output_GetRoomLightShade(const ROOM_LIGHT_MODE mode)
{
    // TODO: remove
    ASSERT_FAIL();
    return 0;
}

void Output_LightRoomVertices(const ROOM *room)
{
    // TODO: remove
    ASSERT_FAIL();
}
