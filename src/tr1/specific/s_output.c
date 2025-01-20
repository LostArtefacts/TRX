#include "specific/s_output.h"

#include "game/output.h"
#include "game/screen.h"
#include "game/shell.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "specific/s_shell.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/gfx/gl/track.h>
#include <libtrx/log.h>

#include <string.h>

#define MAP_DEPTH(zv) (g_FltResZBuf - g_FltResZ * (1.0 / (double)(zv)))

static int m_TextureMap[GFX_MAX_TEXTURES] = { GFX_NO_TEXTURE };
static int m_EnvMapTexture = GFX_NO_TEXTURE;

static GFX_2D_RENDERER *m_Renderer2D = nullptr;
static GFX_3D_RENDERER *m_Renderer3D = nullptr;
static bool m_IsTextureMode = false;
static int32_t m_SelectedTexture = -1;

static int32_t m_SurfaceWidth = 0;
static int32_t m_SurfaceHeight = 0;
static float m_SurfaceMinX = 0.0f;
static float m_SurfaceMinY = 0.0f;
static float m_SurfaceMaxX = 0.0f;
static float m_SurfaceMaxY = 0.0f;
static GFX_2D_SURFACE *m_PrimarySurface = nullptr;
static GFX_2D_SURFACE *m_PictureSurface = nullptr;
static GFX_2D_SURFACE *m_TextureSurfaces[GFX_MAX_TEXTURES] = { nullptr };

static inline float M_GetUV(uint16_t uv);
static void M_ReleaseTextures(void);
static void M_ReleaseSurfaces(void);
static void M_FlipPrimaryBuffer(void);
static void M_ClearSurface(GFX_2D_SURFACE *surface);
static void M_DrawTriangleFan(GFX_3D_VERTEX *vertices, int vertex_count);
static void M_DrawTriangleStrip(GFX_3D_VERTEX *vertices, int vertex_count);

static inline float M_GetUV(const uint16_t uv)
{
    return g_Config.rendering.pretty_pixels
            && g_Config.rendering.texture_filter == GFX_TF_NN
        ? uv / 65536.0f
        : ((uv & 0xFF00) + 127) / 65536.0f;
}

static void M_ReleaseTextures(void)
{
    if (m_Renderer3D == nullptr) {
        return;
    }
    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (m_TextureMap[i] != GFX_NO_TEXTURE) {
            GFX_3D_Renderer_UnregisterTexturePage(
                m_Renderer3D, m_TextureMap[i]);
            m_TextureMap[i] = GFX_NO_TEXTURE;
        }
    }
    if (m_EnvMapTexture != GFX_NO_TEXTURE) {
        GFX_3D_Renderer_UnregisterEnvironmentMap(m_Renderer3D, m_EnvMapTexture);
    }
}

static void M_ReleaseSurfaces(void)
{
    if (m_PrimarySurface) {
        M_ClearSurface(m_PrimarySurface);

        GFX_2D_Surface_Free(m_PrimarySurface);
        m_PrimarySurface = nullptr;
    }

    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        if (m_TextureSurfaces[i] != nullptr) {
            GFX_2D_Surface_Free(m_TextureSurfaces[i]);
            m_TextureSurfaces[i] = nullptr;
        }
    }

    if (m_PictureSurface) {
        GFX_2D_Surface_Free(m_PictureSurface);
        m_PictureSurface = nullptr;
    }
}

void Output_FillEnvironmentMap(void)
{
    GFX_3D_Renderer_FillEnvironmentMap(m_Renderer3D);
}

static void M_FlipPrimaryBuffer(void)
{
    GFX_Context_SwapBuffers();
}

static void M_ClearSurface(GFX_2D_SURFACE *surface)
{
    GFX_2D_Surface_Clear(surface, 0);
}

static void M_DrawTriangleFan(GFX_3D_VERTEX *vertices, int vertex_count)
{
    GFX_3D_Renderer_RenderPrimFan(m_Renderer3D, vertices, vertex_count);
}

static void M_DrawTriangleStrip(GFX_3D_VERTEX *vertices, int vertex_count)
{
    GFX_3D_Renderer_RenderPrimStrip(m_Renderer3D, vertices, vertex_count);
}

void S_Output_EnableTextureMode(void)
{
    if (m_IsTextureMode) {
        return;
    }

    m_IsTextureMode = true;
    GFX_3D_Renderer_SetTexturingEnabled(m_Renderer3D, m_IsTextureMode);
}

void S_Output_DisableTextureMode(void)
{
    if (!m_IsTextureMode) {
        return;
    }

    m_IsTextureMode = false;
    GFX_3D_Renderer_SetTexturingEnabled(m_Renderer3D, m_IsTextureMode);
}

void S_Output_SetBlendingMode(const GFX_BLEND_MODE blend_mode)
{
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, blend_mode);
}

void S_Output_EnableDepthWrites(void)
{
    GFX_3D_Renderer_SetDepthWritesEnabled(m_Renderer3D, true);
}

void S_Output_DisableDepthWrites(void)
{
    GFX_3D_Renderer_SetDepthWritesEnabled(m_Renderer3D, false);
}

void S_Output_EnableDepthTest(void)
{
    GFX_3D_Renderer_SetDepthTestEnabled(m_Renderer3D, true);
}

void S_Output_DisableDepthTest(void)
{
    GFX_3D_Renderer_SetDepthTestEnabled(m_Renderer3D, false);
}

void S_Output_EnableCullFace(void)
{
    GFX_3D_Renderer_SetCullFaceEnabled(m_Renderer3D, true);
}

void S_Output_DisableCullFace(void)
{
    GFX_3D_Renderer_SetCullFaceEnabled(m_Renderer3D, false);
}

void S_Output_RenderBegin(void)
{
    GFX_Context_Clear();
    GFX_Track_Reset();
    S_Output_DrawBackdropSurface();
    GFX_3D_Renderer_RenderBegin(m_Renderer3D);
    GFX_3D_Renderer_SetViewPos(
        m_Renderer3D, g_Camera.pos.x, g_Camera.pos.y, g_Camera.pos.z);
    GFX_3D_Renderer_SetTextureFilter(
        m_Renderer3D, g_Config.rendering.texture_filter);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
}

void S_Output_RenderEnd(void)
{
    GFX_3D_Renderer_RenderEnd(m_Renderer3D);
}

void S_Output_Flush(void)
{
    GFX_3D_Renderer_Flush(m_Renderer3D);
}

void S_Output_FlipScreen(void)
{
    M_FlipPrimaryBuffer();
    m_SelectedTexture = -1;
}

void S_Output_ClearDepthBuffer(void)
{
    GFX_3D_Renderer_ClearDepth(m_Renderer3D);
}

void S_Output_DrawBackdropSurface(void)
{
    if (m_PictureSurface == nullptr) {
        return;
    }
    GFX_2D_Renderer_Render(m_Renderer2D);
}

void S_Output_DownloadBackdropSurface(const IMAGE *const image)
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

void S_Output_SelectTexture(const int32_t texture_num)
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

void S_Output_DrawSprite(
    int16_t x1, int16_t y1, int16_t x2, int y2, int z, int sprnum, int shade)
{
    int vertex_count = 4;
    GFX_3D_VERTEX vertices[vertex_count];

    float multiplier = g_Config.visuals.brightness / 16.0f;

    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(sprnum);
    float vshade = (MAX_LIGHTING - shade) * multiplier;
    if (vshade >= 256.0f) {
        vshade = 255.0f;
    }

    const float u0 = (sprite->offset & 0xFF) / 256.0f;
    const float v0 = (sprite->offset >> 8) / 256.0f;
    const float u1 = (sprite->width >> 8) / 256.0f + u0;
    const float v1 = (sprite->height >> 8) / 256.0f + v0;
    const float vz = MAP_DEPTH(z);
    const float rhw = 1.0f / z;

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = vz;
    vertices[0].s = u0;
    vertices[0].t = v0;
    vertices[0].tex_coord[2] = 1.0;
    vertices[0].tex_coord[3] = 1.0;
    vertices[0].w = rhw;
    vertices[0].r = vshade;
    vertices[0].g = vshade;
    vertices[0].b = vshade;
    vertices[0].flags = 0;

    vertices[1].x = x2;
    vertices[1].y = y1;
    vertices[1].z = vz;
    vertices[1].s = u1;
    vertices[1].t = v0;
    vertices[1].tex_coord[2] = 1.0;
    vertices[1].tex_coord[3] = 1.0;
    vertices[1].w = rhw;
    vertices[1].r = vshade;
    vertices[1].g = vshade;
    vertices[1].b = vshade;
    vertices[1].flags = 0;

    vertices[2].x = x2;
    vertices[2].y = y2;
    vertices[2].z = vz;
    vertices[2].s = u1;
    vertices[2].t = v1;
    vertices[2].tex_coord[2] = 1.0;
    vertices[2].tex_coord[3] = 1.0;
    vertices[2].w = rhw;
    vertices[2].r = vshade;
    vertices[2].g = vshade;
    vertices[2].b = vshade;
    vertices[2].flags = 0;

    vertices[3].x = x1;
    vertices[3].y = y2;
    vertices[3].z = vz;
    vertices[3].s = u0;
    vertices[3].t = v1;
    vertices[3].tex_coord[2] = 1.0;
    vertices[3].tex_coord[3] = 1.0;
    vertices[3].w = rhw;
    vertices[3].r = vshade;
    vertices[3].g = vshade;
    vertices[3].b = vshade;
    vertices[3].flags = 0;

    if (!vertex_count) {
        return;
    }

    if (m_TextureMap[sprite->tex_page] != GFX_NO_TEXTURE) {
        S_Output_EnableTextureMode();
        S_Output_SelectTexture(sprite->tex_page);
        M_DrawTriangleFan(vertices, vertex_count);
    } else {
        S_Output_DisableTextureMode();
        M_DrawTriangleFan(vertices, vertex_count);
    }
}

void S_Output_Draw2DLine(
    const XYZ_32 pos1, const XYZ_32 pos2, const RGBA_8888 color1,
    const RGBA_8888 color2)
{
    int vertex_count = 2;
    GFX_3D_VERTEX vertices[vertex_count];

    vertices[0].x = pos1.x;
    vertices[0].y = pos1.y;
    vertices[0].z = pos1.z;
    vertices[0].r = color1.r;
    vertices[0].g = color1.g;
    vertices[0].b = color1.b;
    vertices[0].a = color1.a;
    vertices[0].flags = 0;

    vertices[1].x = pos2.x;
    vertices[1].y = pos2.y;
    vertices[1].z = pos2.z;
    vertices[1].r = color2.r;
    vertices[1].g = color2.g;
    vertices[1].b = color2.b;
    vertices[1].a = color2.a;
    vertices[1].flags = 0;

    GFX_3D_Renderer_SetPrimType(m_Renderer3D, GFX_3D_PRIM_LINE);
    S_Output_DisableTextureMode();
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_NORMAL);
    GFX_3D_Renderer_RenderPrimList(m_Renderer3D, vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
    GFX_3D_Renderer_SetPrimType(m_Renderer3D, GFX_3D_PRIM_TRI);
}

void S_Output_Draw2DQuad(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br)
{
    int vertex_count = 4;
    GFX_3D_VERTEX vertices[vertex_count];

    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = 1.0f;
    vertices[0].r = tl.r;
    vertices[0].g = tl.g;
    vertices[0].b = tl.b;
    vertices[0].a = tl.a;
    vertices[0].flags = 0;

    vertices[1].x = x2;
    vertices[1].y = y1;
    vertices[1].z = 1.0f;
    vertices[1].r = tr.r;
    vertices[1].g = tr.g;
    vertices[1].b = tr.b;
    vertices[1].a = tr.a;
    vertices[1].flags = 0;

    vertices[2].x = x2;
    vertices[2].y = y2;
    vertices[2].z = 1.0f;
    vertices[2].r = br.r;
    vertices[2].g = br.g;
    vertices[2].b = br.b;
    vertices[2].a = br.a;
    vertices[2].flags = 0;

    vertices[3].x = x1;
    vertices[3].y = y2;
    vertices[3].z = 1.0f;
    vertices[3].r = bl.r;
    vertices[3].g = bl.g;
    vertices[3].b = bl.b;
    vertices[3].a = bl.a;
    vertices[3].flags = 0;

    S_Output_DisableTextureMode();
    S_Output_DisableCullFace();
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_NORMAL);
    M_DrawTriangleFan(vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
    S_Output_EnableCullFace();
}

void S_Output_DrawLightningSegment(
    int x1, int y1, int z1, int thickness1, int x2, int y2, int z2,
    int thickness2)
{
    int vertex_count = 4;
    GFX_3D_VERTEX vertices[vertex_count];

    S_Output_DisableTextureMode();

    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_NORMAL);
    vertices[0].x = x1;
    vertices[0].y = y1;
    vertices[0].z = MAP_DEPTH(z1);
    vertices[0].g = 0.0f;
    vertices[0].r = 0.0f;
    vertices[0].b = 255.0f;
    vertices[0].a = 128.0f;
    vertices[0].flags = 0;

    vertices[1].x = thickness1 / 2 + x1;
    vertices[1].y = vertices[0].y;
    vertices[1].z = vertices[0].z;
    vertices[1].b = 255.0f;
    vertices[1].g = 255.0f;
    vertices[1].r = 255.0f;
    vertices[1].a = 128.0f;
    vertices[1].flags = 0;

    vertices[2].x = thickness2 / 2 + x2;
    vertices[2].y = y2;
    vertices[2].z = MAP_DEPTH(z2);
    vertices[2].b = 255.0f;
    vertices[2].g = 255.0f;
    vertices[2].r = 255.0f;
    vertices[2].a = 128.0f;
    vertices[2].flags = 0;

    vertices[3].x = x2;
    vertices[3].y = vertices[2].y;
    vertices[3].z = vertices[2].z;
    vertices[3].g = 0.0f;
    vertices[3].r = 0.0f;
    vertices[3].b = 255.0f;
    vertices[3].a = 128.0f;
    vertices[3].flags = 0;

    M_DrawTriangleFan(vertices, vertex_count);

    vertex_count = 4;
    vertices[0].x = thickness1 / 2 + x1;
    vertices[0].y = y1;
    vertices[0].z = MAP_DEPTH(z1);
    vertices[0].b = 255.0f;
    vertices[0].g = 255.0f;
    vertices[0].r = 255.0f;
    vertices[0].a = 128.0f;
    vertices[0].flags = 0;

    vertices[1].x = thickness1 + x1;
    vertices[1].y = vertices[0].y;
    vertices[1].z = vertices[0].z;
    vertices[1].g = 0.0f;
    vertices[1].r = 0.0f;
    vertices[1].b = 255.0f;
    vertices[1].a = 128.0f;
    vertices[1].flags = 0;

    vertices[2].x = (thickness2 + x2);
    vertices[2].y = y2;
    vertices[2].z = MAP_DEPTH(z2);
    vertices[2].g = 0.0f;
    vertices[2].r = 0.0f;
    vertices[2].b = 255.0f;
    vertices[2].a = 128.0f;
    vertices[2].flags = 0;

    vertices[3].x = (thickness2 / 2 + x2);
    vertices[3].y = vertices[2].y;
    vertices[3].z = vertices[2].z;
    vertices[3].b = 255.0f;
    vertices[3].g = 255.0f;
    vertices[3].r = 255.0f;
    vertices[3].a = 128.0f;
    vertices[3].flags = 0;

    M_DrawTriangleFan(vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
}

void S_Output_DrawShadow(const int32_t vertex_count, const PHD_VBUF *src_vbuf)
{
    GFX_3D_VERTEX vertices[vertex_count];

    for (int32_t i = 0; i < vertex_count; i++) {
        GFX_3D_VERTEX *vertex = &vertices[i];
        const PHD_VBUF *vbuf = &src_vbuf[i];
        vertex->x = vbuf->world_pos.x;
        vertex->y = vbuf->world_pos.y;
        vertex->z = vbuf->world_pos.z;
        vertex->b = 0.0f;
        vertex->g = 0.0f;
        vertex->r = 0.0f;
        vertex->a = 128.0f;
        vertex->flags = 1;
    }

    S_Output_DisableTextureMode();

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-0.5, 0.0);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_NORMAL);
    M_DrawTriangleFan(vertices, vertex_count);
    S_Output_Flush();
    glDisable(GL_POLYGON_OFFSET_FILL);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
}

void S_Output_ApplyRenderSettings(void)
{
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
    m_SurfaceMinX = 0.0f;
    m_SurfaceMinY = 0.0f;
    m_SurfaceMaxX = Screen_GetResWidth() - 1.0f;
    m_SurfaceMaxY = Screen_GetResHeight() - 1.0f;

    GFX_Context_SetVSync(g_Config.rendering.enable_vsync);
    GFX_Context_SetDisplayFilter(g_Config.rendering.fbo_filter);
    GFX_Context_SetDisplaySize(m_SurfaceWidth, m_SurfaceHeight);
    GFX_Context_SetRenderingMode(g_Config.rendering.render_mode);
    GFX_Context_SetWireframeMode(g_Config.rendering.enable_wireframe);
    GFX_Context_SetLineWidth(g_Config.rendering.wireframe_width);
    GFX_3D_Renderer_SetAnisotropyFilter(
        m_Renderer3D, g_Config.rendering.anisotropy_filter);

    if (m_PrimarySurface == nullptr) {
        GFX_2D_SURFACE_DESC surface_desc = {};
        m_PrimarySurface = GFX_2D_Surface_Create(&surface_desc);
    }
    M_ClearSurface(m_PrimarySurface);
}

void S_Output_SetWindowSize(int width, int height)
{
    GFX_Context_SetWindowSize(width, height);
}

bool S_Output_Init(void)
{
    for (int i = 0; i < GFX_MAX_TEXTURES; i++) {
        m_TextureMap[i] = GFX_NO_TEXTURE;
        m_TextureSurfaces[i] = nullptr;
    }

    m_Renderer2D = GFX_2D_Renderer_Create();
    m_Renderer3D = GFX_3D_Renderer_Create();

    S_Output_ApplyRenderSettings();
    GFX_3D_Renderer_SetPrimType(m_Renderer3D, GFX_3D_PRIM_TRI);
    GFX_3D_Renderer_SetAlphaThreshold(m_Renderer3D, 0.0);
    GFX_3D_Renderer_SetAlphaPointDiscard(m_Renderer3D, true);

    return true;
}

void S_Output_Shutdown(void)
{
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
}

void S_Output_DownloadTextures(int32_t pages)
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

    m_SelectedTexture = -1;

    m_EnvMapTexture = GFX_3D_Renderer_RegisterEnvironmentMap(m_Renderer3D);
}

void S_Output_2ColourBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 col_dark,
    RGBA_8888 col_light, float thickness)
{
#define SET_VERTEX(i, color, x_, y_)                                           \
    screen_box_verticies[(i)].x = (x_);                                        \
    screen_box_verticies[(i)].y = (y_);                                        \
    screen_box_verticies[i].z = 1.0f;                                          \
    screen_box_verticies[i].s = 0.0f;                                          \
    screen_box_verticies[i].t = 0.0f;                                          \
    screen_box_verticies[i].w = 0.0f;                                          \
    screen_box_verticies[i].r = (color).r;                                     \
    screen_box_verticies[i].g = (color).g;                                     \
    screen_box_verticies[i].b = (color).b;                                     \
    screen_box_verticies[i].a = (color).a;                                     \
    screen_box_verticies[i].flags = 0;

    GFX_3D_VERTEX screen_box_verticies[22];

    const float sxf = (float)sx + thickness;
    const float syf = (float)sy + thickness;
    const float hf = (float)h;
    const float wf = (float)w;

    // Top Left Dark set
    SET_VERTEX(0, col_dark, sxf, syf + hf - thickness);
    SET_VERTEX(1, col_dark, sxf + thickness, syf + hf - thickness);
    SET_VERTEX(2, col_dark, sxf, syf);
    SET_VERTEX(3, col_dark, sxf + thickness, syf + thickness);
    SET_VERTEX(4, col_dark, sxf + wf - thickness, syf);
    SET_VERTEX(5, col_dark, sxf + wf - thickness, syf + thickness);

    // Bottom Right Dark set
    SET_VERTEX(6, col_dark, sxf + wf + thickness, syf - thickness);
    SET_VERTEX(7, col_dark, sxf + wf, syf - thickness);
    SET_VERTEX(8, col_dark, sxf + wf + thickness, syf + hf + thickness);
    SET_VERTEX(9, col_dark, sxf + wf, syf + hf);
    SET_VERTEX(10, col_dark, sxf - thickness, syf + hf + thickness);
    SET_VERTEX(11, col_dark, sxf - thickness, syf + hf);

    // Light box
    SET_VERTEX(12, col_light, sxf - thickness, syf + hf);
    SET_VERTEX(13, col_light, sxf, syf + hf - thickness);
    SET_VERTEX(14, col_light, sxf - thickness, syf - thickness);
    SET_VERTEX(15, col_light, sxf, syf);
    SET_VERTEX(16, col_light, sxf + wf, syf - thickness);
    SET_VERTEX(17, col_light, sxf + wf - thickness, syf);
    SET_VERTEX(18, col_light, sxf + wf, syf + hf);
    SET_VERTEX(19, col_light, sxf + wf - thickness, syf + hf - thickness);
    SET_VERTEX(20, col_light, sxf - thickness, syf + hf);
    SET_VERTEX(21, col_light, sxf, syf + hf - thickness);

    S_Output_DisableTextureMode();
    S_Output_DisableCullFace();
    M_DrawTriangleStrip(screen_box_verticies, 22);
    S_Output_EnableCullFace();
}

void S_Output_4ColourTextBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 tl, RGBA_8888 tr,
    RGBA_8888 bl, RGBA_8888 br, float thickness)
{
    //  0                 2
    //   *               &
    //    1             3
    //
    //    7             5
    //   #               @
    //  6                 4
    GFX_3D_VERTEX screen_box_verticies[10];
    for (int i = 0; i < 10; i++) {
        screen_box_verticies[i].z = -1.0f;
        screen_box_verticies[i].s = 0.0f;
        screen_box_verticies[i].t = 0.0f;
        screen_box_verticies[i].w = 0.0f;
        screen_box_verticies[i].flags = 0;
    }
    screen_box_verticies[0].x = sx - thickness;
    screen_box_verticies[0].y = sy - thickness;

    screen_box_verticies[1].x = sx + thickness;
    screen_box_verticies[1].y = sy + thickness;

    screen_box_verticies[0].r = screen_box_verticies[1].r = tl.r;
    screen_box_verticies[0].g = screen_box_verticies[1].g = tl.g;
    screen_box_verticies[0].b = screen_box_verticies[1].b = tl.b;
    screen_box_verticies[0].a = screen_box_verticies[1].a = tl.a;

    screen_box_verticies[2].x = sx + w + thickness;
    screen_box_verticies[2].y = sy - thickness;

    screen_box_verticies[3].x = sx + w - thickness;
    screen_box_verticies[3].y = sy + thickness;

    screen_box_verticies[2].r = screen_box_verticies[3].r = tr.r;
    screen_box_verticies[2].g = screen_box_verticies[3].g = tr.g;
    screen_box_verticies[2].b = screen_box_verticies[3].b = tr.b;
    screen_box_verticies[2].a = screen_box_verticies[3].a = tr.a;

    screen_box_verticies[4].x = sx + w + thickness;
    screen_box_verticies[4].y = sy + h + thickness;

    screen_box_verticies[5].x = sx + w - thickness;
    screen_box_verticies[5].y = sy + h - thickness;

    screen_box_verticies[4].r = screen_box_verticies[5].r = br.r;
    screen_box_verticies[4].g = screen_box_verticies[5].g = br.g;
    screen_box_verticies[4].b = screen_box_verticies[5].b = br.b;
    screen_box_verticies[4].a = screen_box_verticies[5].a = br.a;

    screen_box_verticies[6].x = sx - thickness;
    screen_box_verticies[6].y = sy + h + thickness;

    screen_box_verticies[7].x = sx + thickness;
    screen_box_verticies[7].y = sy + h - thickness;

    screen_box_verticies[6].r = screen_box_verticies[7].r = bl.r;
    screen_box_verticies[6].g = screen_box_verticies[7].g = bl.g;
    screen_box_verticies[6].b = screen_box_verticies[7].b = bl.b;
    screen_box_verticies[6].a = screen_box_verticies[7].a = bl.a;

    screen_box_verticies[8].x = screen_box_verticies[0].x;
    screen_box_verticies[8].y = screen_box_verticies[0].y;

    screen_box_verticies[9].x = screen_box_verticies[1].x;
    screen_box_verticies[9].y = screen_box_verticies[1].y;

    screen_box_verticies[8].r = screen_box_verticies[9].r = tl.r;
    screen_box_verticies[8].g = screen_box_verticies[9].g = tl.g;
    screen_box_verticies[8].b = screen_box_verticies[9].b = tl.b;
    screen_box_verticies[8].a = screen_box_verticies[9].a = tl.a;

    S_Output_DisableTextureMode();
    S_Output_DisableCullFace();
    M_DrawTriangleStrip(screen_box_verticies, 10);
    S_Output_EnableCullFace();
}

void S_Output_2ToneColourTextBox(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGBA_8888 edge,
    RGBA_8888 centre, float thickness)
{
    //  0        2        4
    //   *               &
    //    1      3      5
    //
    // 14 15            7 6
    //
    //    13    10      9
    //   #               @
    // 12       11        8

    int32_t halfw = w / 2;
    int32_t halfh = h / 2;

    GFX_3D_VERTEX screen_box_verticies[18];
    for (int i = 0; i < 18; i++) {
        screen_box_verticies[i].z = 1.0f;
        screen_box_verticies[i].s = 0.0f;
        screen_box_verticies[i].t = 0.0f;
        screen_box_verticies[i].w = 0.0f;
        screen_box_verticies[i].flags = 0;
    }

    screen_box_verticies[0].x = sx - thickness;
    screen_box_verticies[0].y = sy - thickness;

    screen_box_verticies[1].x = sx + thickness;
    screen_box_verticies[1].y = sy + thickness;

    screen_box_verticies[0].r = screen_box_verticies[1].r = edge.r;
    screen_box_verticies[0].g = screen_box_verticies[1].g = edge.g;
    screen_box_verticies[0].b = screen_box_verticies[1].b = edge.b;
    screen_box_verticies[0].a = screen_box_verticies[1].a = edge.a;

    screen_box_verticies[2].x = sx + halfw;
    screen_box_verticies[2].y = sy - thickness;

    screen_box_verticies[3].x = sx + halfw;
    screen_box_verticies[3].y = sy + thickness;

    screen_box_verticies[2].r = screen_box_verticies[3].r = centre.r;
    screen_box_verticies[2].g = screen_box_verticies[3].g = centre.g;
    screen_box_verticies[2].b = screen_box_verticies[3].b = centre.b;
    screen_box_verticies[2].a = screen_box_verticies[3].a = centre.a;

    screen_box_verticies[4].x = sx + w + thickness;
    screen_box_verticies[4].y = sy - thickness;

    screen_box_verticies[5].x = sx + w - thickness;
    screen_box_verticies[5].y = sy + thickness;

    screen_box_verticies[4].r = screen_box_verticies[5].r = edge.r;
    screen_box_verticies[4].g = screen_box_verticies[5].g = edge.g;
    screen_box_verticies[4].b = screen_box_verticies[5].b = edge.b;
    screen_box_verticies[4].a = screen_box_verticies[5].a = edge.a;

    screen_box_verticies[6].x = sx + w + thickness;
    screen_box_verticies[6].y = sy + halfh;

    screen_box_verticies[7].x = sx + w - thickness;
    screen_box_verticies[7].y = sy + halfh;

    screen_box_verticies[6].r = screen_box_verticies[7].r = centre.r;
    screen_box_verticies[6].g = screen_box_verticies[7].g = centre.g;
    screen_box_verticies[6].b = screen_box_verticies[7].b = centre.b;
    screen_box_verticies[6].a = screen_box_verticies[7].a = centre.a;

    screen_box_verticies[8].x = sx + w + thickness;
    screen_box_verticies[8].y = sy + h + thickness;

    screen_box_verticies[9].x = sx + w - thickness;
    screen_box_verticies[9].y = sy + h - thickness;

    screen_box_verticies[8].r = screen_box_verticies[9].r = edge.r;
    screen_box_verticies[8].g = screen_box_verticies[9].g = edge.g;
    screen_box_verticies[8].b = screen_box_verticies[9].b = edge.b;
    screen_box_verticies[8].a = screen_box_verticies[9].a = edge.a;

    screen_box_verticies[10].x = sx + halfw;
    screen_box_verticies[10].y = sy + h + thickness;

    screen_box_verticies[11].x = sx + halfw;
    screen_box_verticies[11].y = sy + h - thickness;

    screen_box_verticies[10].r = screen_box_verticies[11].r = centre.r;
    screen_box_verticies[10].g = screen_box_verticies[11].g = centre.g;
    screen_box_verticies[10].b = screen_box_verticies[11].b = centre.b;
    screen_box_verticies[10].a = screen_box_verticies[11].a = centre.a;

    screen_box_verticies[12].x = sx - thickness;
    screen_box_verticies[12].y = sy + h + thickness;

    screen_box_verticies[13].x = sx + thickness;
    screen_box_verticies[13].y = sy + h - thickness;

    screen_box_verticies[12].r = screen_box_verticies[13].r = edge.r;
    screen_box_verticies[12].g = screen_box_verticies[13].g = edge.g;
    screen_box_verticies[12].b = screen_box_verticies[13].b = edge.b;
    screen_box_verticies[12].a = screen_box_verticies[13].a = edge.a;

    screen_box_verticies[14].x = sx - thickness;
    screen_box_verticies[14].y = sy + halfh;

    screen_box_verticies[15].x = sx + thickness;
    screen_box_verticies[15].y = sy + halfh;

    screen_box_verticies[14].r = screen_box_verticies[15].r = centre.r;
    screen_box_verticies[14].g = screen_box_verticies[15].g = centre.g;
    screen_box_verticies[14].b = screen_box_verticies[15].b = centre.b;
    screen_box_verticies[14].a = screen_box_verticies[15].a = centre.a;

    screen_box_verticies[16].x = screen_box_verticies[0].x;
    screen_box_verticies[16].y = screen_box_verticies[0].y;

    screen_box_verticies[17].x = screen_box_verticies[1].x;
    screen_box_verticies[17].y = screen_box_verticies[1].y;

    screen_box_verticies[16].r = screen_box_verticies[17].r = edge.r;
    screen_box_verticies[16].g = screen_box_verticies[17].g = edge.g;
    screen_box_verticies[16].b = screen_box_verticies[17].b = edge.b;
    screen_box_verticies[16].a = screen_box_verticies[17].a = edge.a;

    S_Output_DisableTextureMode();
    S_Output_DisableCullFace();
    M_DrawTriangleStrip(screen_box_verticies, 18);
    S_Output_EnableCullFace();
}

void S_Output_DrawFlat(
    const int32_t vertex_count, const PHD_VBUF *src_vbuf[],
    const RGBA_8888 color)
{
    GFX_3D_VERTEX vertices[vertex_count];
    const float multiplier = g_Config.visuals.brightness / (16.0f * 255.0f);
    for (int32_t i = 0; i < vertex_count; i++) {
        vertices[i].x = src_vbuf[i]->world_pos.x;
        vertices[i].y = src_vbuf[i]->world_pos.y;
        vertices[i].z = src_vbuf[i]->world_pos.z;
        const float light = (MAX_LIGHTING - src_vbuf[i]->g) * multiplier;
        vertices[i].r = color.r * light;
        vertices[i].g = color.g * light;
        vertices[i].b = color.b * light;
        vertices[i].a = color.a;
        Output_ApplyTint(&vertices[i].r, &vertices[i].g, &vertices[i].b);
        vertices[i].flags = src_vbuf[i]->flags;
    }
    S_Output_DisableTextureMode();
    M_DrawTriangleFan(vertices, vertex_count);
}

void S_Output_DrawTextured(
    const int32_t vertex_count, const PHD_VBUF *src_vbuf[],
    const int16_t tex_page)
{
    GFX_3D_VERTEX vertices[vertex_count];
    const float multiplier = g_Config.visuals.brightness / 16.0f;
    for (int32_t i = 0; i < vertex_count; i++) {
        vertices[i].x = src_vbuf[i]->world_pos.x;
        vertices[i].y = src_vbuf[i]->world_pos.y;
        vertices[i].z = src_vbuf[i]->world_pos.z;
        vertices[i].w = 1.0f;
        vertices[i].s = M_GetUV(src_vbuf[i]->u);
        vertices[i].t = M_GetUV(src_vbuf[i]->v);
        vertices[i].tex_coord[2] = src_vbuf[i]->tex_coord[2];
        vertices[i].tex_coord[3] = src_vbuf[i]->tex_coord[3];
        vertices[i].r = vertices[i].g = vertices[i].b =
            (MAX_LIGHTING - src_vbuf[i]->g) * multiplier;
        Output_ApplyTint(&vertices[i].r, &vertices[i].g, &vertices[i].b);
        vertices[i].flags = src_vbuf[i]->flags;
    }
    if (m_TextureMap[tex_page] != GFX_NO_TEXTURE) {
        S_Output_EnableTextureMode();
        S_Output_SelectTexture(tex_page);
    } else {
        S_Output_DisableTextureMode();
    }
    GFX_3D_Renderer_RenderPrimFan(m_Renderer3D, vertices, vertex_count);
}

void S_Output_DrawEnvMap(const int32_t vertex_count, const PHD_VBUF *src_vbuf[])
{
    GFX_3D_VERTEX vertices[vertex_count];
    const float multiplier = g_Config.visuals.brightness / 16.0f;
    for (int32_t i = 0; i < vertex_count; i++) {
        vertices[i].x = src_vbuf[i]->world_pos.x;
        vertices[i].y = src_vbuf[i]->world_pos.y;
        vertices[i].z = src_vbuf[i]->world_pos.z;
        vertices[i].w = 1.0f;
        vertices[i].s = M_GetUV(src_vbuf[i]->u);
        vertices[i].t = M_GetUV(src_vbuf[i]->v);
        vertices[i].tex_coord[2] = src_vbuf[i]->tex_coord[2];
        vertices[i].tex_coord[3] = src_vbuf[i]->tex_coord[3];
        vertices[i].r = vertices[i].g = vertices[i].b =
            (MAX_LIGHTING - src_vbuf[i]->g) * multiplier;
        Output_ApplyTint(&vertices[i].r, &vertices[i].g, &vertices[i].b);
        vertices[i].flags = src_vbuf[i]->flags;
    }
    S_Output_EnableTextureMode();
    GFX_3D_Renderer_SelectTexture(m_Renderer3D, m_EnvMapTexture);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_MULTIPLY);
    GFX_3D_Renderer_RenderPrimStrip(m_Renderer3D, vertices, vertex_count);
    GFX_3D_Renderer_SetBlendingMode(m_Renderer3D, GFX_BLEND_MODE_OFF);
    m_SelectedTexture = -1;
}

void S_Output_UploadModelMatrix(void)
{
    GLfloat model_view[4][4];
    Output_GetModelMatrix(model_view);
    S_Output_Flush();
    GFX_3D_Renderer_SetModelView(m_Renderer3D, model_view);
}

void S_Output_UploadProjectionMatrix(void)
{
    GLfloat projection[4][4];
    Output_GetProjectionMatrix(projection);
    S_Output_Flush();
    GFX_3D_Renderer_SetProjection(m_Renderer3D, projection);
}

void S_Output_ResetMatrix(void)
{
    S_Output_Flush();
    GFX_3D_Renderer_SetIdentityMatrix(m_Renderer3D);
}

void S_Output_SetWibbleEffect(const bool enabled, const float offset)
{
    GFX_3D_Renderer_SetWibbleOffset(m_Renderer3D, enabled ? offset : -1.0f);
}

GLuint Output_GetTexturePageBinding(const int32_t page)
{
    return GFX_3D_Renderer_GetTextureBinding(m_Renderer3D, m_TextureMap[page]);
}
