#include "game/output.h"

#include "game/output/meshes/common.h"
#include "game/output/meshes/dynamic.h"
#include "game/output/meshes/objects.h"
#include "game/output/meshes/rooms.h"
#include "game/output/sprites.h"
#include "game/output/textures.h"
#include "game/output/utils.h"
#include "game/overlay.h"
#include "game/screen.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "specific/s_shell.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/engine/image.h>
#include <libtrx/filesystem.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/output.h>
#include <libtrx/gfx/gl/track.h>
#include <libtrx/gfx/gl/utils.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>

#include <string.h>

#define MAX_LIGHTNINGS 64
#define MAP_DEPTH(zv) (g_FltResZBuf - g_FltResZ * (1.0 / (double)(zv)))

typedef struct {
    XYZ_32 pos_0;
    XYZ_32 pos_1;
    int32_t thickness;
} LIGHTNING;

static int32_t m_LsAdder = 0;
static int32_t m_LsDivider = 0;
static bool m_IsSkyboxEnabled = false;
static bool m_IsWibbleEffect = false;
static bool m_IsWaterEffect = false;
static bool m_IsShadeEffect = false;

static int32_t m_Time = 0;
static int32_t m_AnimatedTexturesOffset = 0;

static int32_t m_DrawDistFade = 0;
static int32_t m_DrawDistMax = 0;
static RGB_F m_WaterColor = {};
static XYZ_32 m_LsVectorView = {};

static int32_t m_LightningCount = 0;
static LIGHTNING m_LightningTable[MAX_LIGHTNINGS];
static int32_t m_TextureMap[GFX_MAX_TEXTURES] = { GFX_NO_TEXTURE };

static GFX_2D_RENDERER *m_Renderer2D = nullptr;
static GFX_3D_RENDERER *m_Renderer3D = nullptr;
static bool m_IsTextureMode = false;
static int32_t m_SelectedTexture = -1;

static int32_t m_SurfaceWidth = 0;
static int32_t m_SurfaceHeight = 0;
static GFX_2D_SURFACE *m_PictureSurface = nullptr;
static GFX_2D_SURFACE *m_TextureSurfaces[GFX_MAX_TEXTURES] = { nullptr };

static char *m_BackdropImagePath = nullptr;
static const char *m_ImageExtensions[] = {
    ".png", ".jpg", ".jpeg", ".pcx", nullptr,
};

static struct {
    GLint bound_program;
    GLint bound_vao;
    GLint bound_vbo;
    GLint bound_active_texture;
    GLint bound_texture;
    GLint bound_polygon_mode[2];
} m_CachedState;

static inline float M_GetUV(uint16_t uv);
static void M_SelectTexture(int32_t texture_num);
static void M_EnableDepthWrites(void);
static void M_DisableDepthWrites(void);
static void M_EnableDepthTest(void);
static void M_DisableDepthTest(void);
static void M_EnableTextureMode(void);
static void M_DisableTextureMode(void);
static void M_DownloadBackdropSurface(const IMAGE *image);
static void M_DownloadTextures(int32_t pages);
static void M_ReleaseTextures(void);
static void M_ReleaseSurfaces(void);
static void M_Flush(void);
static void M_FlipScreen(void);

static void M_DrawTriangleFan(
    const GFX_3D_VERTEX *vertices, int32_t vertex_count);
static void M_DrawTriangleStrip(
    const GFX_3D_VERTEX *vertices, int32_t vertex_count);
static void M_Draw2DQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br);
static void M_DrawSphere(XYZ_32 pos, int32_t radius);
static void M_DrawLightningSegment(const LIGHTNING *const lightning);
static void M_DrawSprite(
    int16_t x1, int16_t y1, int16_t x2, int32_t y2, int32_t z,
    int32_t sprite_idx, int16_t shade);

static inline float M_GetUV(const uint16_t uv)
{
    return g_Config.rendering.pretty_pixels
            && g_Config.rendering.texture_filter == GFX_TF_NN
        ? uv / 65536.0f
        : ((uv & 0xFF00) + 127) / 65536.0f;
}

static void M_SelectTexture(const int32_t texture_num)
{
    if (texture_num == m_SelectedTexture) {
        return;
    }
    if (m_TextureMap[texture_num] == GFX_NO_TEXTURE) {
        LOG_ERROR("ERROR: Attempt to select unloaded texture");
        return;
    }
    GFX_3D_Renderer_SelectTexture(m_Renderer3D, m_TextureMap[texture_num]);
    m_SelectedTexture = texture_num;
}

static void M_EnableDepthWrites(void)
{
    GFX_3D_Renderer_SetDepthWritesEnabled(m_Renderer3D, true);
}

static void M_DisableDepthWrites(void)
{
    GFX_3D_Renderer_SetDepthWritesEnabled(m_Renderer3D, false);
}

static void M_EnableDepthTest(void)
{
    GFX_3D_Renderer_SetDepthTestEnabled(m_Renderer3D, true);
}

static void M_DisableDepthTest(void)
{
    GFX_3D_Renderer_SetDepthTestEnabled(m_Renderer3D, false);
}

static void M_EnableTextureMode(void)
{
    if (!m_IsTextureMode) {
        m_IsTextureMode = true;
        GFX_3D_Renderer_SetTexturingEnabled(m_Renderer3D, m_IsTextureMode);
    }
}

static void M_DisableTextureMode(void)
{
    if (m_IsTextureMode) {
        m_IsTextureMode = false;
        GFX_3D_Renderer_SetTexturingEnabled(m_Renderer3D, m_IsTextureMode);
    }
}

static void M_DownloadBackdropSurface(const IMAGE *const image)
{
    GFX_2D_Surface_Free(m_PictureSurface);
    m_PictureSurface = nullptr;
    if (image == nullptr) {
        return;
    }
    m_PictureSurface = GFX_2D_Surface_CreateFromImage(image);
    GFX_2D_Renderer_Upload(
        m_Renderer2D, &m_PictureSurface->desc, m_PictureSurface->buffer);
}

static void M_DownloadTextures(const int32_t pages)
{
    if (pages > GFX_MAX_TEXTURES) {
        Shell_ExitSystem("Attempt to download more than texture page limit");
    }
    M_ReleaseTextures();

    for (int32_t i = 0; i < pages; i++) {
        if (m_TextureSurfaces[i] == nullptr) {
            const GFX_2D_SURFACE_DESC surface_desc = {
                .width = TEXTURE_PAGE_WIDTH,
                .height = TEXTURE_PAGE_HEIGHT,
            };
            m_TextureSurfaces[i] = GFX_2D_Surface_Create(&surface_desc);
        }
        GFX_2D_SURFACE *const surface = m_TextureSurfaces[i];
        RGBA_8888 *const output_ptr = (RGBA_8888 *)surface->buffer;
        const RGBA_8888 *const input_ptr = Output_GetTexturePage32(i);
        memcpy(
            output_ptr, input_ptr,
            surface->desc.width * surface->desc.height * sizeof(RGBA_8888));
        m_TextureMap[i] = GFX_3D_Renderer_RegisterTexturePage(
            m_Renderer3D, output_ptr, surface->desc.width,
            surface->desc.height);
    }
}

static void M_ReleaseTextures(void)
{
    if (m_Renderer3D == nullptr) {
        return;
    }
    for (int32_t i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (m_TextureMap[i] != GFX_NO_TEXTURE) {
            GFX_3D_Renderer_UnregisterTexturePage(
                m_Renderer3D, m_TextureMap[i]);
            m_TextureMap[i] = GFX_NO_TEXTURE;
        }
    }
    m_SelectedTexture = -1;
}

static void M_ReleaseSurfaces(void)
{
    for (int32_t i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (m_TextureSurfaces[i] != nullptr) {
            GFX_2D_Surface_Free(m_TextureSurfaces[i]);
            m_TextureSurfaces[i] = nullptr;
        }
    }
    if (m_PictureSurface != nullptr) {
        GFX_2D_Surface_Free(m_PictureSurface);
        m_PictureSurface = nullptr;
    }
}

static void M_Flush(void)
{
    GFX_3D_Renderer_Flush(m_Renderer3D);
}

static void M_FlipScreen(void)
{
    GFX_Context_SwapBuffers();
    m_SelectedTexture = -1;
}

static void M_DrawBackdropSurface(void)
{
}

static void M_DrawTriangleFan(
    const GFX_3D_VERTEX *const vertices, const int32_t vertex_count)
{
    GFX_3D_Renderer_RenderPrimFan(m_Renderer3D, vertices, vertex_count);
}

static void M_DrawTriangleStrip(
    const GFX_3D_VERTEX *const vertices, const int32_t vertex_count)
{
    GFX_3D_Renderer_RenderPrimStrip(m_Renderer3D, vertices, vertex_count);
}

static void M_Draw2DQuad(
    const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2,
    const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
    const RGBA_8888 br)
{
    int32_t vertex_count = 4;
    GFX_3D_VERTEX vertices[vertex_count];

#define SET(vtx_idx, x_, y_, z_, color)                                        \
    vertices[vtx_idx].x = x_, vertices[vtx_idx].y = y_,                        \
    vertices[vtx_idx].z = 1.0f, vertices[vtx_idx].r = color.r;                 \
    vertices[vtx_idx].g = color.g;                                             \
    vertices[vtx_idx].b = color.b;                                             \
    vertices[vtx_idx].a = color.a;
    SET(0, x1, y1, 1.0f, tl);
    SET(1, x2, y1, 1.0f, tr);
    SET(2, x2, y2, 1.0f, br);
    SET(3, x1, y2, 1.0f, bl);
#undef SET

    M_DisableTextureMode();
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_NORMAL);
    M_DrawTriangleFan(vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
}

static void M_DrawSphere(const XYZ_32 pos, const int32_t radius)
{
    // More subdivisions means smoother spheres.
    const int32_t subdivisions = 12;
    const int32_t position_count = SQUARE(subdivisions + 1);
    XYZ_F positions[position_count];
    int32_t index = 0;

    for (int32_t i = 0; i <= subdivisions; i++) {
        const float theta = (M_PI * i) / subdivisions; // Latitude angle
        const float sin_theta = sinf(theta);
        const float cos_theta = cosf(theta);

        for (int32_t j = 0; j <= subdivisions; j++) {
            const float phi = (2 * M_PI * j) / subdivisions; // Longitude angle
            const float sin_phi = sinf(phi);
            const float cos_phi = cosf(phi);

            // Convert spherical coordinates to 3D points.
            positions[index] = (XYZ_F) {
                .x = pos.x + radius * cos_phi * sin_theta,
                .y = pos.y + radius * cos_theta,
                .z = pos.z + radius * sin_phi * sin_theta,
            };
            index++;
        }
    }

    const int32_t vertex_count =
        subdivisions * subdivisions * OUTPUT_QUAD_VERTICES;
    OUTPUT_MESH_VERTEX vertices[vertex_count];
    OUTPUT_MESH_VERTEX *out_vertex = vertices;
    for (int32_t i = 0; i < subdivisions; i++) {
        for (int32_t j = 0; j < subdivisions; j++) {
            const int32_t indices[4] = {
                i * (subdivisions + 1) + j,
                (i + 1) * (subdivisions + 1) + j,
                (i + 1) * (subdivisions + 1) + (j + 1),
                i * (subdivisions + 1) + (j + 1),
            };
            for (int32_t k = 0; k < OUTPUT_QUAD_VERTICES; k++) {
                out_vertex->pos = positions[indices[OUTPUT_QUAD_TO_FAN(k)]];
                out_vertex++;
            }
        }
    }

    const RGBA_8888 color_black = { 0, 0, 0, 128 };
    const RGBA_8888 color_white = { 255, 255, 255, 128 };
    const bool wireframe_state = GFX_Context_GetWireframeMode();
    for (int32_t i = 0; i < vertex_count; i++) {
        vertices[i].flags = VERT_FLAT_SHADED | VERT_NO_LIGHTING;
        vertices[i].color = wireframe_state ? color_black : color_white;
    }

    glGetIntegerv(GL_POLYGON_MODE, &m_CachedState.bound_polygon_mode[0]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Output_Shader_UploadMatrix(Output_Meshes_GetShader(), g_MatrixPtr);
    Output_Meshes_DrawTriangles(vertex_count, vertices);
    glBlendFunc(GL_ONE, GL_ZERO);
    glPolygonMode(GL_FRONT_AND_BACK, m_CachedState.bound_polygon_mode[0]);
}

static void M_DrawLightningSegment(const LIGHTNING *const lightning)
{
    const int32_t vertex_count = 8;
    const RGB_F blue = { 0.0f, 0.0f, 255.0f };
    const RGB_F white = { 255.0f, 255.0f, 255.0f };
    GFX_3D_VERTEX vertices[vertex_count];

    const XYZ_32 wp0 = lightning->pos_0;
    const XYZ_32 wp1 = lightning->pos_1;
    if (wp0.z < Output_GetNearZ() || wp0.z > Output_GetFarZ()
        || wp1.z < Output_GetNearZ() || wp1.z > Output_GetFarZ()) {
        return;
    }

    const int32_t vcx = Viewport_GetCenterX();
    const int32_t vcy = Viewport_GetCenterY();
    const float zp0 = wp0.z / g_PhdPersp;
    const float zp1 = wp1.z / g_PhdPersp;
    const XYZ_32 p0 = { vcx + wp0.x / zp0, vcy + wp0.y / zp0, wp0.z };
    const XYZ_32 p1 = { vcx + wp1.x / zp1, vcy + wp1.y / zp1, wp1.z };
    const int32_t t1 = (lightning->thickness << W2V_SHIFT) / zp0;
    const int32_t t2 = (lightning->thickness << W2V_SHIFT) / zp1;

#define SET(vtx_idx, x_, y_, z_, color)                                        \
    vertices[vtx_idx].x = x_;                                                  \
    vertices[vtx_idx].y = y_;                                                  \
    vertices[vtx_idx].z = MAP_DEPTH(z_);                                       \
    vertices[vtx_idx].r = color.r;                                             \
    vertices[vtx_idx].g = color.g;                                             \
    vertices[vtx_idx].b = color.b;                                             \
    vertices[vtx_idx].a = 128.0f;
    // clang-format off
    LOG_INFO("%d %d %d, %d %d %d, %d", p0.x, p0.y, p0.z, p1.x, p1.y, p1.z, t1);
    SET(0, p0.x,          p0.y, p0.z, blue);
    SET(1, p0.x + t1 / 2, p0.y, p0.z, white);
    SET(2, p1.x + t2 / 2, p1.y, p1.z, white);
    SET(3, p1.x,          p1.y, p1.z, blue);
    SET(4, p0.x + t1 / 2, p0.y, p0.z, white);
    SET(5, p0.x + t1,     p0.y, p0.z, blue);
    SET(6, p1.x + t2,     p1.y, p1.z, blue);
    SET(7, p1.x + t2 / 2, p1.y, p1.z, white);
    // clang-format on
#undef SET

    M_DisableTextureMode();
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_NORMAL);
    M_DrawTriangleFan(&vertices[0], 4);
    M_DrawTriangleFan(&vertices[4], 4);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
}

static void M_DrawSprite(
    const int16_t x1, const int16_t y1, const int16_t x2, const int32_t y2,
    const int32_t z, const int32_t sprite_idx, const int16_t shade)
{
    const int32_t vertex_count = 4;
    GFX_3D_VERTEX vertices[vertex_count];

    const float multiplier = g_Config.visuals.brightness / 16.0f;
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);
    float vshade = (MAX_LIGHTING - shade) * multiplier;
    CLAMPG(vshade, 255.0f);

    const float u0 = (sprite->offset & 0xFF) / 256.0f;
    const float v0 = (sprite->offset >> 8) / 256.0f;
    const float u1 = (sprite->width >> 8) / 256.0f + u0;
    const float v1 = (sprite->height >> 8) / 256.0f + v0;
    const float vz = MAP_DEPTH(z);
    const float rhw = 1.0f / z;

#define SET(vtx_idx, x_, y_, z_, u_, v_)                                       \
    vertices[vtx_idx].x = x_;                                                  \
    vertices[vtx_idx].y = y_;                                                  \
    vertices[vtx_idx].z = z_;                                                  \
    vertices[vtx_idx].s = u_;                                                  \
    vertices[vtx_idx].t = v_;                                                  \
    vertices[vtx_idx].tex_coord[2] = 1.0;                                      \
    vertices[vtx_idx].tex_coord[3] = 1.0;                                      \
    vertices[vtx_idx].w = rhw;                                                 \
    vertices[vtx_idx].r = vshade;                                              \
    vertices[vtx_idx].g = vshade;                                              \
    vertices[vtx_idx].b = vshade;

    SET(0, x1, y1, vz, u0, v0);
    SET(1, x2, y1, vz, u1, v0);
    SET(2, x2, y2, vz, u1, v1);
    SET(3, x1, y2, vz, u0, v1);
#undef SET

    if (m_TextureMap[sprite->tex_page] != GFX_NO_TEXTURE) {
        M_EnableTextureMode();
        M_SelectTexture(sprite->tex_page);
        M_DrawTriangleFan(vertices, vertex_count);
    } else {
        M_DisableTextureMode();
        M_DrawTriangleFan(vertices, vertex_count);
    }
}

bool Output_Init(void)
{
    for (int32_t i = 0; i < GFX_MAX_TEXTURES; i++) {
        m_TextureMap[i] = GFX_NO_TEXTURE;
        m_TextureSurfaces[i] = nullptr;
    }

    m_Renderer2D = GFX_2D_Renderer_Create();
    m_Renderer3D = GFX_3D_Renderer_Create();

    Output_ApplyRenderSettings();
    GFX_3D_Renderer_SetPrimType(m_Renderer3D, GFX_3D_PRIM_TRI);
    GFX_3D_Renderer_SetAlphaThreshold(m_Renderer3D, 0.0);
    GFX_3D_Renderer_SetAlphaPointDiscard(m_Renderer3D, true);

    Output_Textures_Init();
    Output_Sprites_Init();
    Output_Meshes_Init();
    return true;
}

void Output_Shutdown(void)
{
    Output_Meshes_Shutdown();
    Output_Sprites_Shutdown();
    Output_Textures_Shutdown();

    M_ReleaseTextures();
    M_ReleaseSurfaces();

    if (m_Renderer2D != nullptr) {
        GFX_2D_Renderer_Destroy(m_Renderer2D);
        m_Renderer2D = nullptr;
    }
    if (m_Renderer3D != nullptr) {
        GFX_3D_Renderer_Destroy(m_Renderer3D);
        m_Renderer3D = nullptr;
    }
    GFX_Context_Detach();
    Memory_FreePointer(&m_BackdropImagePath);
}

void Output_SetWindowSize(int32_t width, int32_t height)
{
    GFX_Context_SetWindowSize(width, height);
}

void Output_ApplyRenderSettings(void)
{
    Output_Textures_ApplyRenderSettings();

    if (m_Renderer3D == nullptr) {
        return;
    }

    if (m_PictureSurface != nullptr
        && (Screen_GetResWidth() != m_SurfaceWidth
            || Screen_GetResHeight() != m_SurfaceHeight)) {
        GFX_2D_Surface_Free(m_PictureSurface);
        m_PictureSurface = nullptr;
    }

    m_SurfaceWidth = Screen_GetResWidth();
    m_SurfaceHeight = Screen_GetResHeight();

    GFX_Context_SetVSync(g_Config.rendering.enable_vsync);
    GFX_Context_SetDisplayFilter(g_Config.rendering.fbo_filter);
    GFX_Context_SetDisplaySize(m_SurfaceWidth, m_SurfaceHeight);
    GFX_Context_SetRenderingMode(g_Config.rendering.render_mode);
    GFX_Context_SetWireframeMode(g_Config.rendering.enable_wireframe);
    GFX_Context_SetLineWidth(g_Config.rendering.wireframe_width);
    GFX_3D_Renderer_SetAnisotropyFilter(
        m_Renderer3D, g_Config.rendering.anisotropy_filter);

    if (m_BackdropImagePath != nullptr) {
        Output_LoadBackgroundFromFile(m_BackdropImagePath);
    }
}

void Output_ObserveLevelLoad(void)
{
    M_DownloadTextures(Output_GetTexturePageCount());
    Output_Textures_ObserveLevelLoad();
    Output_Sprites_ObserveLevelLoad();
    Output_Meshes_ObserveLevelLoad();
}

void Output_ObserveLevelUnload(void)
{
    Output_Meshes_ObserveLevelUnload();
}

void Output_ObserveFOVChange(void)
{
    Output_Meshes_UploadProjectionMatrix();
    Output_Sprites_UploadProjectionMatrix();
}

void Output_FlushTranslucentObjects(void)
{
    Output_RememberState();
    // draw transparent lightnings as last in the 3D geometry pipeline
    for (int32_t i = 0; i < m_LightningCount; i++) {
        M_DrawLightningSegment(&m_LightningTable[i]);
    }
    Output_RestoreState();
}

void Output_BeginScene(void)
{
    Output_ApplyFOV();
    Text_DrawReset();

    Output_RememberState();
    Output_Sprites_RenderBegin();
    Output_Meshes_RenderBegin();

    GFX_Context_Clear();
    GFX_Track_Reset();
    GFX_3D_Renderer_RenderBegin(m_Renderer3D);
    GFX_3D_Renderer_SetTextureFilter(
        m_Renderer3D, g_Config.rendering.texture_filter);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
    m_LightningCount = 0;
}

void Output_EndScene(void)
{
    M_DisableDepthTest();
    Output_ClearDepthBuffer();
    Overlay_DrawFPSInfo();
    M_EnableDepthTest();
    GFX_3D_Renderer_RenderEnd(m_Renderer3D);
    M_FlipScreen();
    Shell_ProcessEvents();
    g_FPSCounter++;
}

void Output_ClearDepthBuffer(void)
{
    GFX_3D_Renderer_ClearDepth(m_Renderer3D);
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
    M_DisableDepthTest();
    Output_RememberState();
    Output_Meshes_RenderObjectMesh(g_MatrixPtr, Output_GetTint(), mesh);
    Output_RestoreState();
    M_EnableDepthTest();
}

void Output_DrawObjectMesh(const OBJECT_MESH *const mesh, const int32_t clip)
{
    M_Flush();
    Output_RememberState();
    Output_Meshes_RenderObjectMesh(g_MatrixPtr, Output_GetTint(), mesh);
    Output_RestoreState();

    if (g_Config.rendering.enable_debug_spheres) {
        M_DrawSphere(
            (XYZ_32) { mesh->center.x, mesh->center.y, mesh->center.z },
            mesh->radius);
    }
    M_Flush();
}

void Output_DrawObjectMesh_I(const OBJECT_MESH *const mesh, const int32_t clip)
{
    Matrix_Push();
    Matrix_Interpolate();
    Output_DrawObjectMesh(mesh, clip);
    Matrix_Pop();
}

void Output_DrawRoomMesh(ROOM *const room)
{
    Output_RememberState();
    Output_LightRoom(room);
    Output_EnableScissor(
        room->bound_left, room->bound_bottom,
        room->bound_right - room->bound_left,
        room->bound_bottom - room->bound_top);
    Output_Meshes_RenderRoomMesh(g_MatrixPtr, Output_GetTint(), room);
    Output_DisableScissor();
    Output_RestoreState();
}

void Output_DrawRoomPortals(const ROOM *const room)
{
    const int32_t vertex_count = room->portals->count * 8;
    OUTPUT_MESH_VERTEX vertices[vertex_count];
    OUTPUT_MESH_VERTEX *out_vertex = vertices;
    const RGBA_8888 color = { 0, 0, 255, 255 };
    for (int32_t i = 0; i < room->portals->count; i++) {
        const PORTAL *const portal = &room->portals->portal[i];
        const XYZ_F positions[4] = {
            { portal->vertex[0].x, portal->vertex[0].y, portal->vertex[0].z },
            { portal->vertex[1].x, portal->vertex[1].y, portal->vertex[1].z },
            { portal->vertex[2].x, portal->vertex[2].y, portal->vertex[2].z },
            { portal->vertex[3].x, portal->vertex[3].y, portal->vertex[3].z },
        };
        const int32_t indices[8] = { 0, 1, 1, 2, 2, 3, 3, 0 };
        for (int32_t j = 0; j < 8; j++) {
            out_vertex->pos = positions[indices[j]];
            out_vertex++;
        }
    }
    for (int32_t i = 0; i < vertex_count; i++) {
        vertices[i].uvw_idx = -1;
        vertices[i].flags = VERT_FLAT_SHADED | VERT_NO_LIGHTING;
        vertices[i].color = color;
    }

    glDisable(GL_DEPTH_TEST);
    glGetIntegerv(GL_POLYGON_MODE, &m_CachedState.bound_polygon_mode[0]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Output_Shader_UploadMatrix(Output_Meshes_GetShader(), g_MatrixPtr);
    Output_Meshes_DrawPrimitives(GL_LINES, vertex_count, vertices);
    glBlendFunc(GL_ONE, GL_ZERO);
    glPolygonMode(GL_FRONT_AND_BACK, m_CachedState.bound_polygon_mode[0]);
    glEnable(GL_DEPTH_TEST);
}

void Output_DrawRoomTriggers(const ROOM *const room)
{
    const RGBA_8888 color = { .r = 255, .g = 0, .b = 255, .a = 128 };
    const XZ_16 offsets[4] = { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };

    int32_t vertex_count = 0;
    for (int32_t z = 0; z < room->size.z; z++) {
        for (int32_t x = 0; x < room->size.x; x++) {
            const SECTOR *sector = Room_GetUnitSector(room, x, z);
            if (sector->trigger == nullptr) {
                continue;
            }
            vertex_count += OUTPUT_QUAD_VERTICES;
        }
    }

    OUTPUT_MESH_VERTEX *vertices =
        Memory_Alloc(vertex_count * sizeof(OUTPUT_MESH_VERTEX));
    OUTPUT_MESH_VERTEX *out_vertex = vertices;

    for (int32_t z = 0; z < room->size.z; z++) {
        for (int32_t x = 0; x < room->size.x; x++) {
            const SECTOR *sector = Room_GetUnitSector(room, x, z);
            if (sector->trigger == nullptr) {
                continue;
            }
            for (int32_t i = 0; i < OUTPUT_QUAD_VERTICES; i++) {
                const int32_t j = OUTPUT_QUAD_TO_FAN(i);
                XYZ_16 vertex_pos = {
                    .x = (x + offsets[j].x) * WALL_L,
                    .z = (z + offsets[j].z) * WALL_L,
                };
                XYZ_32 world_pos = {
                    .x = room->pos.x + x * WALL_L + offsets[j].x * (WALL_L - 1),
                    .z = room->pos.z + z * WALL_L + offsets[j].z * (WALL_L - 1),
                    .y = room->pos.y,
                };

                int16_t room_num = room - Room_Get(0);
                sector = Room_GetSector(
                    world_pos.x, world_pos.y, world_pos.z, &room_num);
                vertex_pos.y =
                    Room_GetHeight(
                        sector, world_pos.x, world_pos.y, world_pos.z)
                    + (m_IsWaterEffect ? -16 : -2);

                out_vertex->pos =
                    (XYZ_F) { vertex_pos.x, vertex_pos.y, vertex_pos.z };
                out_vertex->flags = VERT_FLAT_SHADED | VERT_NO_LIGHTING;
                out_vertex->color = color;
                out_vertex++;
            }
        }
    }

    M_DisableTextureMode();
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Output_Shader_UploadMatrix(Output_Meshes_GetShader(), g_MatrixPtr);
    Output_Meshes_DrawTriangles(vertex_count, vertices);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDepthMask(GL_TRUE);
    Memory_FreePointer(&vertices);
}

void Output_DrawShadow(
    const int16_t size, const BOUNDS_16 *const bounds, const ITEM *const item)
{
    if (!item->enable_shadow) {
        return;
    }

    const int32_t vertex_count = g_Config.visuals.enable_round_shadow ? 32 : 8;
    const int32_t x0 = bounds->min.x;
    const int32_t x1 = bounds->max.x;
    const int32_t z0 = bounds->min.z;
    const int32_t z1 = bounds->max.z;
    const int32_t x_mid = (x0 + x1) / 2;
    const int32_t z_mid = (z0 + z1) / 2;
    const int32_t x_add = (x1 - x0) * size / 1024;
    const int32_t z_add = (z1 - z0) * size / 1024;

    Matrix_Push();
    Matrix_TranslateAbs(
        item->interp.result.pos.x, item->floor, item->interp.result.pos.z);
    Matrix_RotY(item->rot.y);

    OUTPUT_MESH_VERTEX vertices[vertex_count * 3];
    for (int32_t i = 0; i < vertex_count; i++) {
        for (int32_t j = 0; j < 2; j++) {
            const int32_t angle = (DEG_180 + (i + j) * DEG_360) / vertex_count;
            vertices[i * 3 + j].pos.x =
                x_mid + ((x_add * 2) * Math_Sin(angle) >> W2V_SHIFT);
            vertices[i * 3 + j].pos.z =
                z_mid + ((z_add * 2) * Math_Cos(angle) >> W2V_SHIFT);
        }
        vertices[i * 3 + 2].pos.x = x_mid;
        vertices[i * 3 + 2].pos.z = z_mid;
        for (int32_t j = 0; j < 3; j++) {
            vertices[i * 3 + j].pos.y = -5;
            vertices[i * 3 + j].flags = VERT_FLAT_SHADED | VERT_NO_LIGHTING;
            vertices[i * 3 + j].color = (RGBA_8888) { 0, 0, 0, 128 };
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Output_Shader_UploadMatrix(Output_Meshes_GetShader(), g_MatrixPtr);
    Output_Meshes_DrawTriangles(vertex_count * 3, vertices);
    glBlendFunc(GL_ONE, GL_ZERO);

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

void Output_SetDrawDistFade(const int32_t dist)
{
    m_DrawDistFade = dist;
}

void Output_SetDrawDistMax(const int32_t dist)
{
    m_DrawDistMax = dist;

    const double near_z = Output_GetNearZ();
    const double far_z = Output_GetFarZ();
    const double res_z = 0.99 * near_z * far_z / (far_z - near_z);
    g_FltResZ = res_z;
    g_FltResZBuf = 0.005 + res_z / near_z;
}

void Output_SetWaterColor(const RGB_F *const color)
{
    m_WaterColor.r = color->r;
    m_WaterColor.g = color->g;
    m_WaterColor.b = color->b;
}

void Output_SetLightAdder(const int32_t adder)
{
    m_LsAdder = adder;
}

int32_t Output_GetLightAdder(void)
{
    return m_LsAdder;
}

void Output_SetLightDivider(const int32_t divider)
{
    m_LsDivider = divider;
}

int32_t Output_GetLightDivider(void)
{
    return m_LsDivider;
}

XYZ_32 Output_GetLightVectorView(void)
{
    return m_LsVectorView;
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
    const int32_t x, const int32_t y, const int32_t z, const int16_t sprite_idx,
    const int16_t shade)
{
    Matrix_Push();
    Matrix_TranslateAbs(x, y, z);
    Output_Sprites_RenderSingleSprite(
        g_MatrixPtr, (XYZ_32) { 0, 0, 0 }, sprite_idx, shade);
    Matrix_Pop();
}

void Output_DrawScreenFlatQuad(
    const int32_t sx, const int32_t sy, const int32_t w, const int32_t h,
    const RGBA_8888 color)
{
    M_Draw2DQuad(sx, sy, sx + w, sy + h, color, color, color, color);
}

void Output_DrawScreenGradientQuad(
    const int32_t sx, const int32_t sy, const int32_t w, const int32_t h,
    const RGBA_8888 tl, const RGBA_8888 tr, const RGBA_8888 bl,
    const RGBA_8888 br)
{
    M_Draw2DQuad(sx, sy, sx + w, sy + h, tl, tr, bl, br);
}

void Output_DrawScreenFrame(
    int32_t sx, int32_t sy, const int32_t w, const int32_t h,
    const RGBA_8888 col_dark, const RGBA_8888 col_light,
    const int32_t thickness_i)
{
    const float scale = Viewport_GetHeight() / 480.0;
    const float thickness = thickness_i * scale / 2.0f;
    sx -= scale;
    sy -= scale;

#define SB_NUM_VERTS_DARK 12
#define SB_NUM_VERTS_LIGHT 10
    GFX_3D_VERTEX vertices[SB_NUM_VERTS_DARK + SB_NUM_VERTS_LIGHT];
    GFX_3D_VERTEX *const dark_vertices = vertices;
    GFX_3D_VERTEX *const light_vertices = vertices + SB_NUM_VERTS_DARK;
    const float sxf = sx + thickness;
    const float syf = sy + thickness;
    const float hf = h;
    const float wf = w;

#define SET(i, x_, y_)                                                         \
    vertices[i].x = x_;                                                        \
    vertices[i].y = y_;
    // clang-format off
    // Top Left Dark edge
    SET(0,  sxf,                         syf + hf - thickness);
    SET(1,  sxf + thickness,             syf + hf - thickness);
    SET(2,  sxf,                         syf);
    SET(3,  sxf + thickness,             syf + thickness);
    SET(4,  sxf + wf - thickness,        syf);
    SET(5,  sxf + wf - thickness,        syf + thickness);
    // Bottom Right Dark set
    SET(6,  sxf + wf + thickness,        syf - thickness);
    SET(7,  sxf + wf,                    syf - thickness);
    SET(8,  sxf + wf + thickness,        syf + hf + thickness);
    SET(9,  sxf + wf,                    syf + hf);
    SET(10, sxf - thickness,             syf + hf + thickness);
    SET(11, sxf - thickness,             syf + hf);
    // Light box
    SET(12, sxf - thickness,             syf + hf);
    SET(13, sxf - thickness + thickness, syf + hf - thickness);
    SET(14, sxf - thickness,             syf - thickness);
    SET(15, sxf,                         syf);
    SET(16, sxf + wf,                    syf - thickness);
    SET(17, sxf + wf - thickness,        syf);
    SET(18, sxf + wf,                    syf + hf);
    SET(19, sxf + wf - thickness,        syf + hf - thickness);
    SET(20, sxf - thickness,             syf + hf);
    SET(21, sxf - thickness + thickness, syf + hf - thickness);
    // clang-format on
#undef SET

    for (int32_t i = 0; i < SB_NUM_VERTS_DARK + SB_NUM_VERTS_LIGHT; i++) {
        vertices[i].z = 1.0f;
        vertices[i].s = 0.0f;
        vertices[i].t = 0.0f;
        vertices[i].w = 0.0f;
    }
    for (int32_t i = 0; i < SB_NUM_VERTS_DARK; i++) {
        dark_vertices[i].r = col_dark.r;
        dark_vertices[i].g = col_dark.g;
        dark_vertices[i].b = col_dark.b;
        dark_vertices[i].a = col_dark.a;
    }
    for (int32_t i = 0; i < SB_NUM_VERTS_LIGHT; i++) {
        light_vertices[i].r = col_light.r;
        light_vertices[i].g = col_light.g;
        light_vertices[i].b = col_light.b;
        light_vertices[i].a = col_light.a;
    }
    M_DisableTextureMode();
    M_DrawTriangleStrip(vertices, SB_NUM_VERTS_DARK + SB_NUM_VERTS_LIGHT);
}

void Output_DrawGradientScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, const RGBA_8888 tl,
    const RGBA_8888 tr, const RGBA_8888 bl, const RGBA_8888 br,
    const int32_t thickness_i)
{
    const float scale = Viewport_GetHeight() / 480.0;
    const float thickness = thickness_i * scale / 2.0f;
    sx -= scale;
    sy -= scale;
    w += scale;
    h += scale;

    //  0                 2
    //   *               &
    //    1             3
    //
    //    7             5
    //   #               @
    //  6                 4
    GFX_3D_VERTEX vertices[10];

#define SET(i, x_, y_, color)                                                  \
    vertices[i].x = x_;                                                        \
    vertices[i].y = y_;                                                        \
    vertices[i].r = color.r;                                                   \
    vertices[i].g = color.g;                                                   \
    vertices[i].b = color.b;                                                   \
    vertices[i].a = color.a;
    // clang-format off
    SET(0, sx - thickness,     sy - thickness,     tl);
    SET(1, sx + thickness,     sy + thickness,     tl);
    SET(2, sx + w + thickness, sy - thickness,     tr);
    SET(3, sx + w - thickness, sy + thickness,     tr);
    SET(4, sx + w + thickness, sy + h + thickness, br);
    SET(5, sx + w - thickness, sy + h - thickness, br);
    SET(6, sx - thickness,     sy + h + thickness, bl);
    SET(7, sx + thickness,     sy + h - thickness, bl);
    SET(8, sx - thickness,     sy - thickness,     tl);
    SET(9, sx + thickness,     sy + thickness,     tl);
    // clang-format on
#undef SET

    for (int32_t i = 0; i < 10; i++) {
        vertices[i].z = 1.0f;
        vertices[i].s = 0.0f;
        vertices[i].t = 0.0f;
        vertices[i].w = 0.0f;
    }
    M_DisableTextureMode();
    M_DrawTriangleStrip(vertices, 10);
}

void Output_DrawCentreGradientScreenBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, const RGBA_8888 edge,
    const RGBA_8888 center, const int32_t thickness_i)
{
    const float scale = Viewport_GetHeight() / 480.0;
    const float thickness = thickness_i * scale / 2.0f;
    sx -= scale;
    sy -= scale;
    w += scale;
    h += scale;

    //  0        2        4
    //   *               &
    //    1      3      5
    //
    // 14 15            7 6
    //
    //    13    10      9
    //   #               @
    // 12       11        8

    const int32_t half_w = w / 2;
    const int32_t half_h = h / 2;
    GFX_3D_VERTEX vertices[18];

#define SET(i, x_, y_, color)                                                  \
    vertices[i].x = x_;                                                        \
    vertices[i].y = y_;                                                        \
    vertices[i].r = color.r;                                                   \
    vertices[i].g = color.g;                                                   \
    vertices[i].b = color.b;                                                   \
    vertices[i].a = color.a;
    // clang-format off
    SET(0, sx - thickness,     sy - thickness,     edge);
    SET(1, sx + thickness,     sy + thickness,     edge);
    SET(2, sx + half_w,        sy - thickness,     center);
    SET(3, sx + half_w,        sy + thickness,     center);
    SET(4, sx + w + thickness, sy - thickness,     edge);
    SET(5, sx + w - thickness, sy + thickness,     edge);
    SET(6, sx + w + thickness, sy + half_h,        center);
    SET(7, sx + w - thickness, sy + half_h,        center);
    SET(8, sx + w + thickness, sy + h + thickness, edge);
    SET(9, sx + w - thickness, sy + h - thickness, edge);
    SET(10, sx + half_w,       sy + h + thickness, center);
    SET(11, sx + half_w,       sy + h - thickness, center);
    SET(12, sx - thickness,    sy + h + thickness, edge);
    SET(13, sx + thickness,    sy + h - thickness, edge);
    SET(14, sx - thickness,    sy + half_h,        center);
    SET(15, sx + thickness,    sy + half_h,        center);
    SET(16, sx - thickness,    sy - thickness,     edge);
    SET(17, sx + thickness,    sy + thickness,     edge);
    // clang-format on
#undef SET

    for (int32_t i = 0; i < 18; i++) {
        vertices[i].z = 1.0f;
        vertices[i].s = 0.0f;
        vertices[i].t = 0.0f;
        vertices[i].w = 0.0f;
    }
    M_DisableTextureMode();
    M_DrawTriangleStrip(vertices, 18);
}

void Output_DrawScreenFBox(
    const int32_t sx, const int32_t sy, const int32_t w, const int32_t h)
{
    RGBA_8888 color = { 0, 0, 0, 128 };
    M_Draw2DQuad(sx, sy, sx + w, sy + h, color, color, color, color);
}

void Output_DrawScreenSprite(
    const int32_t sx, const int32_t sy, const int32_t z, const int32_t scale_h,
    const int32_t scale_v, const int32_t sprite_idx, const int16_t shade,
    const uint16_t flags, const int32_t page)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);
    const int32_t x0 = sx + (scale_h * sprite->x0 / PHD_ONE);
    const int32_t x1 = sx + (scale_h * sprite->x1 / PHD_ONE);
    const int32_t y0 = sy + (scale_v * sprite->y0 / PHD_ONE);
    const int32_t y1 = sy + (scale_v * sprite->y1 / PHD_ONE);
    if (x1 >= 0 && y1 >= 0 && x0 < Viewport_GetWidth()
        && y0 < Viewport_GetHeight()) {
        M_DrawSprite(x0, y0, x1, y1, Output_GetNearZ() + 200, sprite_idx, 0);
    }
}

void Output_DrawUISprite(
    const int32_t x, const int32_t y, const int32_t scale,
    const int16_t sprite_idx, const int16_t shade)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprite_idx);
    const int32_t x0 = x + (scale * sprite->x0 >> 16);
    const int32_t x1 = x + (scale * sprite->x1 >> 16);
    const int32_t y0 = y + (scale * sprite->y0 >> 16);
    const int32_t y1 = y + (scale * sprite->y1 >> 16);
    if (x1 >= Viewport_GetMinX() && y1 >= Viewport_GetMinY()
        && x0 <= Viewport_GetMaxX() && y0 <= Viewport_GetMaxY()) {
        M_DrawSprite(
            x0, y0, x1, y1, Output_GetNearZ() + 200, sprite_idx, shade);
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
    M_DownloadBackdropSurface(img);
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
    M_DownloadBackdropSurface(nullptr);
    Memory_FreePointer(&m_BackdropImagePath);
}

void Output_DrawLightningSegment(
    const XYZ_32 pos_0, const XYZ_32 pos_1, const int32_t thickness)
{
    if (m_LightningCount >= MAX_LIGHTNINGS) {
        return;
    }
    LIGHTNING *const lightning = &m_LightningTable[m_LightningCount];
    lightning->pos_0 = pos_0;
    lightning->pos_1 = pos_1;
    lightning->thickness = thickness;
    m_LightningCount++;
}

void Output_SetupBelowWater(const bool underwater)
{
    m_IsWaterEffect = true;
    m_IsWibbleEffect = !underwater;
    m_IsShadeEffect = true;
    Output_RememberState();
    Output_Shader_UploadWaterEffect(Output_Meshes_GetShader(), m_IsWaterEffect);
    Output_RestoreState();
}

void Output_SetupAboveWater(const bool underwater)
{
    m_IsWaterEffect = false;
    m_IsWibbleEffect = underwater;
    m_IsShadeEffect = underwater;
    Output_RememberState();
    Output_Shader_UploadWaterEffect(Output_Meshes_GetShader(), m_IsWaterEffect);
    Output_RestoreState();
}

void Output_AnimateTextures(const int32_t num_frames)
{
    m_Time += num_frames;
    m_AnimatedTexturesOffset += num_frames;
    bool update = false;
    while (m_AnimatedTexturesOffset > 5) {
        Output_CycleAnimatedTextures();
        update = true;
        m_AnimatedTexturesOffset -= 5;
    }
    if (update) {
        Output_Textures_CycleAnimations();
    }
}

void Output_RotateLight(const int16_t pitch, const int16_t yaw)
{
    const int32_t cp = Math_Cos(pitch);
    const int32_t sp = Math_Sin(pitch);
    const int32_t cy = Math_Cos(yaw);
    const int32_t sy = Math_Sin(yaw);
    const int32_t x = TRIGMULT2(cp, sy);
    const int32_t y = -sp;
    const int32_t z = TRIGMULT2(cp, cy);
    const MATRIX *const m = &g_W2VMatrix;
    m_LsVectorView.x = (m->_00 * x + m->_01 * y + m->_02 * z) >> W2V_SHIFT;
    m_LsVectorView.y = (m->_10 * x + m->_11 * y + m->_12 * z) >> W2V_SHIFT;
    m_LsVectorView.z = (m->_20 * x + m->_21 * y + m->_22 * z) >> W2V_SHIFT;
}

void Output_DrawBlackRectangle(const int32_t opacity)
{
    int32_t sx = 0;
    int32_t sy = 0;
    int32_t sw = Viewport_GetWidth();
    int32_t sh = Viewport_GetHeight();

    RGBA_8888 background = { 0, 0, 0, opacity };
    M_DisableDepthTest();
    Output_ClearDepthBuffer();
    Output_DrawScreenFlatQuad(sx, sy, sw, sh, background);
    M_EnableDepthTest();
}

void Output_DrawBackground(void)
{
    if (m_PictureSurface == nullptr) {
        return;
    }
    GFX_2D_Renderer_Render(m_Renderer2D);
}

void Output_DrawPolyList(void)
{
    // force flush the vertex stream
    Output_ClearDepthBuffer();
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

RGB_F Output_GetTint(void)
{
    if (m_IsShadeEffect) {
        return m_WaterColor;
    }
    return (RGB_F) { 1.0f, 1.0f, 1.0f };
}

bool Output_MakeScreenshot(const char *const path)
{
    GFX_Context_ScheduleScreenshot(path);
    return true;
}

int32_t Output_GetObjectBounds(const BOUNDS_16 *const bounds)
{
    // TODO: remove
    return 1;
}

int32_t Output_CalcFogShade(const int32_t depth)
{
    // TODO: done in the shader
    return 0;
}

int32_t Output_GetRoomLightShade(const ROOM_LIGHT_MODE mode)
{
    // TODO: remove
    ASSERT_FAIL();
    return 0;
}

void Output_LightRoomVertices(const ROOM *const room)
{
    // TODO: remove
    ASSERT_FAIL();
}

bool Output_GetWaterEffect(void)
{
    return m_IsWaterEffect;
}

bool Output_GetWibbleEffect(void)
{
    return m_IsWibbleEffect;
}

int32_t Output_GetTime(void)
{
    return m_Time;
}

void Output_GetProjectionMatrix(GLfloat output[][4])
{
    const float left = 0.0f;
    const float top = 0.0f;
    const float right = Viewport_GetWidth();
    const float bottom = Viewport_GetHeight();
    const float near = Output_GetNearZ() / (float)(1 << W2V_SHIFT);
    const float far = Output_GetFarZ() / (float)(1 << W2V_SHIFT);
    const float aspect = (float)right / (float)bottom;
    const float fov = Viewport_GetFOV() * M_PI / (float)DEG_180;
    const float f = 1.0f / tan(fov / 2.0f);

    output[0][0] = f / aspect;
    output[0][1] = 0.0f;
    output[0][2] = 0.0f;
    output[0][3] = 0.0f;

    output[1][0] = 0.0f;
    output[1][1] = -f;
    output[1][2] = 0.0f;
    output[1][3] = 0.0f;

    output[2][0] = 0.0f;
    output[2][1] = 0.0f;
    output[2][2] = g_FltResZBuf;
    output[2][3] = -g_FltResZ / (float)(1 << W2V_SHIFT);

    output[3][0] = 0.0f;
    output[3][1] = 0.0f;
    output[3][2] = 1.0f;
    output[3][3] = 0.0f;
}

void Output_EnableScissor(
    const float x, const float y, const float w, const float h)
{
    // Causes the rendering pipeline to discard every pixel outside of the
    // specified window. The window is in game framebuffer viewport's
    // coordinates; to make it work properly, we need to translate it to the
    // SDL window coordinates first.

    const int32_t border = 2; // to deal with precision issues

    struct {
        GLint x, y, w, h;
    } game_viewport = {
        .x = Viewport_GetMinX(),
        .y = Viewport_GetMinY(),
        .w = Viewport_GetWidth(),
        .h = Viewport_GetHeight(),
    }, gl_viewport, scissor;
    glGetIntegerv(GL_VIEWPORT, &gl_viewport.x);

    const float scale_x = gl_viewport.w / (float)game_viewport.w;
    const float scale_y = gl_viewport.h / (float)game_viewport.h;
    scissor.x = gl_viewport.x + (x * scale_x) - border;
    scissor.y = gl_viewport.y + (game_viewport.h - y) * scale_y - border;
    scissor.w = w * scale_x + border * 2;
    scissor.h = h * scale_y + border * 2;

    glEnable(GL_SCISSOR_TEST);
    glScissor(scissor.x, scissor.y, scissor.w, scissor.h);
}

void Output_DisableScissor(void)
{
    glDisable(GL_SCISSOR_TEST);
}

void Output_RememberState(void)
{
    glGetIntegerv(GL_CURRENT_PROGRAM, &m_CachedState.bound_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_CachedState.bound_vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &m_CachedState.bound_vbo);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &m_CachedState.bound_active_texture);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_CachedState.bound_texture);
    glGetIntegerv(GL_POLYGON_MODE, &m_CachedState.bound_polygon_mode[0]);
    GFX_GL_CheckError();
}

void Output_RestoreState(void)
{
    glUseProgram(m_CachedState.bound_program);
    glBindVertexArray(m_CachedState.bound_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_CachedState.bound_vbo);
    glActiveTexture(m_CachedState.bound_active_texture);
    glBindTexture(GL_TEXTURE_2D, m_CachedState.bound_texture);
    glPolygonMode(GL_FRONT_AND_BACK, m_CachedState.bound_polygon_mode[0]);
    GFX_GL_CheckError();
}

float Output_AdjustUV(const uint16_t uv)
{
    return M_GetUV(uv);
}
