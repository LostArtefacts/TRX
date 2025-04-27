#include "game/output.h"

#include "game/level.h"
#include "game/output/meshes/common.h"
#include "game/output/meshes/rooms.h"
#include "game/output/sprites.h"
#include "game/output/textures.h"
#include "game/overlay.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/filesystem.h>
#include <libtrx/memory.h>

#define MAX_LIGHTNINGS 64
#define MAP_DEPTH(zv) (g_FltResZBuf - g_FltResZ * (1.0 / (double)(zv)))
#define TEXT_OUTLINE_THICKNESS 2

typedef enum {
    MC_PURPLE_C,
    MC_PURPLE_E,
    MC_BROWN_C,
    MC_BROWN_E,
    MC_GREY_C,
    MC_GREY_E,
    MC_GREY_TL,
    MC_GREY_TR,
    MC_GREY_BL,
    MC_GREY_BR,
    MC_BLACK,
    MC_GOLD_LIGHT,
    MC_GOLD_DARK,
    MC_NUMBER_OF,
} MENU_COLOR;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
} QUAD_INFO;

typedef struct {
    XYZ_32 pos_0;
    XYZ_32 pos_1;
    int32_t thickness;
} LIGHTNING;

static bool m_Initialized = false;
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

static const char *m_ImageExtensions[] = {
    ".png", ".jpg", ".jpeg", ".pcx", nullptr,
};

static RGBA_8888 m_MenuColorMap[MC_NUMBER_OF] = {
    { 70, 30, 107, 230 }, // MC_PURPLE_C
    { 70, 30, 107, 0 }, // MC_PURPLE_E
    { 91, 46, 9, 255 }, // MC_BROWN_C
    { 91, 46, 9, 0 }, // MC_BROWN_E
    { 197, 197, 197, 255 }, // MC_GREY_C
    { 45, 45, 45, 255 }, // MC_GREY_E
    { 96, 96, 96, 255 }, // MC_GREY_TL
    { 32, 32, 32, 255 }, // MC_GREY_TR
    { 63, 63, 63, 255 }, // MC_GREY_BL
    { 0, 0, 0, 255 }, // MC_GREY_BR
    { 0, 0, 0, 255 }, // MC_BLACK
    { 232, 192, 112, 255 }, // MC_GOLD_LIGHT
    { 140, 112, 56, 255 }, // MC_GOLD_DARK
};

static RGBA_8888 M_GetMenuColor(MENU_COLOR color);
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
static void M_DrawLightningSegment(const LIGHTNING *const lightning);
static void M_DrawSprite(
    int16_t x1, int16_t y1, int16_t x2, int32_t y2, int32_t z,
    int32_t sprite_idx, int16_t shade);

static RGBA_8888 M_GetMenuColor(MENU_COLOR color)
{
    return m_MenuColorMap[color];
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

#define SET(vtx_idx, x_, y_, color)                                            \
    vertices[vtx_idx].x = x_;                                                  \
    vertices[vtx_idx].y = y_;                                                  \
    vertices[vtx_idx].z = 1.0f;                                                \
    vertices[vtx_idx].r = color.r;                                             \
    vertices[vtx_idx].g = color.g;                                             \
    vertices[vtx_idx].b = color.b;                                             \
    vertices[vtx_idx].a = color.a;
    SET(0, x1, y1, tl);
    SET(1, x2, y1, tr);
    SET(2, x2, y2, br);
    SET(3, x1, y2, bl);
#undef SET

    M_DisableTextureMode();
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_NORMAL);
    M_DrawTriangleFan(vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
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
    float vshade = (SHADE_MAX - shade) * multiplier;
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
    if (m_Initialized) {
        return true;
    }
    m_Initialized = true;

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
    Output_Meshes_Init();
    Output_Sprites_Init();
    return true;
}

void Output_Shutdown(void)
{
    if (!m_Initialized) {
        return;
    }
    m_Initialized = false;

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
    Output_ClearLastBackgroundPath();
}

void Output_SetWindowSize(int32_t width, int32_t height)
{
    GFX_Context_SetWindowSize(width, height);
}

void Output_ApplyLevelSettings(void)
{
    Output_SetWaterColor(Level_GetWaterColor());
    Output_SetFogStart(Level_GetFogStart() * WALL_L);
    Output_SetFogEnd(Level_GetFogEnd() * WALL_L);
}

void Output_ApplyRenderSettings(void)
{
    Output_Textures_ApplyRenderSettings();
    Output_ApplyLevelSettings();

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

    const char *const last_path = Output_GetLastBackgroundPath();
    if (last_path != nullptr) {
        Output_LoadBackgroundFromFile(last_path);
    }
}

void Output_ObserveLevelLoad(void)
{
    M_DownloadTextures(Output_GetTexturePageCount());
    Output_Textures_ObserveLevelLoad();
    Output_Sprites_ObserveLevelLoad();
    Output_Meshes_ObserveLevelLoad();

    Output_ApplyLevelSettings();
}

void Output_ObserveLevelUnload(void)
{
    Output_Meshes_ObserveLevelUnload();
}

void Output_ObserveRoomFlip(const ROOM *room)
{
    Output_Meshes_ObserveRoomFlip(room);
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

bool Output_LoadBackgroundFromImage(const IMAGE *const image)
{
    M_DownloadBackdropSurface(image);
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
    Output_ClearLastBackgroundPath();
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

void Output_DrawBlackRectangle(const int32_t opacity)
{
    const int32_t sx = 0;
    const int32_t sy = 0;
    const int32_t sw = Viewport_GetWidth();
    const int32_t sh = Viewport_GetHeight();
    const RGBA_8888 background = { 0, 0, 0, opacity };
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

void Output_DrawTextBackground(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy, int32_t w,
    int32_t h, const int32_t z, const TEXT_STYLE text_style)
{
    if (ui_style == UI_STYLE_PC) {
        Output_DrawScreenFBox(sx, sy, w, h);
        return;
    }

    // Make sure height and width divisible by 2.
    w = 2 * ((w + 1) / 2);
    h = 2 * ((h + 1) / 2);
    Output_DrawScreenFBox(sx - 1, sy - 1, w + 1, h + 1);

    QUAD_INFO gradient_quads[4] = {
        { sx, sy, w / 2, h / 2 },
        { sx + w, sy, -w / 2, h / 2 },
        { sx, sy + h, w / 2, -h / 2 },
        { sx + w, sy + h, -w / 2, -h / 2 },
    };

    if (text_style == TS_HEADING) {
        for (int i = 0; i < 4; i++) {
            Output_DrawScreenGradientQuad(
                gradient_quads[i].x, gradient_quads[i].y, gradient_quads[i].w,
                gradient_quads[i].h, M_GetMenuColor(MC_BROWN_E),
                M_GetMenuColor(MC_BROWN_E), M_GetMenuColor(MC_BROWN_E),
                M_GetMenuColor(MC_BROWN_C));
        }
    } else if (text_style == TS_REQUESTED) {
        for (int i = 0; i < 4; i++) {
            Output_DrawScreenGradientQuad(
                gradient_quads[i].x, gradient_quads[i].y, gradient_quads[i].w,
                gradient_quads[i].h, M_GetMenuColor(MC_PURPLE_E),
                M_GetMenuColor(MC_PURPLE_E), M_GetMenuColor(MC_PURPLE_E),
                M_GetMenuColor(MC_PURPLE_C));
        }
    }
}

void Output_DrawTextOutline(
    const UI_STYLE ui_style, const int32_t sx, const int32_t sy, int32_t w,
    int32_t h, const int32_t z, const TEXT_STYLE text_style)
{
    if (ui_style == UI_STYLE_PC) {
        Output_DrawScreenFrame(
            sx, sy, w, h, M_GetMenuColor(MC_GOLD_DARK),
            M_GetMenuColor(MC_GOLD_LIGHT), TEXT_OUTLINE_THICKNESS);
        return;
    }

    if (text_style == TS_HEADING) {
        Output_DrawGradientScreenBox(
            sx, sy, w, h, M_GetMenuColor(MC_BLACK), M_GetMenuColor(MC_BLACK),
            M_GetMenuColor(MC_BLACK), M_GetMenuColor(MC_BLACK),
            TEXT_OUTLINE_THICKNESS);
    } else if (text_style == TS_BACKGROUND) {
        Output_DrawGradientScreenBox(
            sx, sy, w, h, M_GetMenuColor(MC_GREY_TL),
            M_GetMenuColor(MC_GREY_TR), M_GetMenuColor(MC_GREY_BL),
            M_GetMenuColor(MC_GREY_BR), TEXT_OUTLINE_THICKNESS);
    } else if (text_style == TS_REQUESTED) {
        // Make sure height and width divisible by 2.
        w = 2 * ((w + 1) / 2);
        h = 2 * ((h + 1) / 2);
        Output_DrawCentreGradientScreenBox(
            sx, sy, w, h, M_GetMenuColor(MC_GREY_E), M_GetMenuColor(MC_GREY_C),
            TEXT_OUTLINE_THICKNESS);
    }
}
