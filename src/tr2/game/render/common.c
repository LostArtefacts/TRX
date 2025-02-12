#include "game/render/common.h"

#include "decomp/decomp.h"
#include "game/render/hwr.h"
#include "game/render/priv.h"
#include "game/render/swr.h"
#include "game/shell.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/gfx/fade/fade_renderer.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>

static RENDERER m_Renderer_SW = {};
static RENDERER m_Renderer_HW = {};
static RENDERER *m_PreviousRenderer = nullptr;
static GFX_FADE_RENDERER *m_FadeRenderer = nullptr;
static GFX_2D_RENDERER *m_BackgroundRenderer = nullptr;

static struct {
    bool ready;
    GFX_2D_SURFACE *surface;
    const OBJECT_TEXTURE *texture;
    int32_t repeat_x;
    int32_t repeat_y;
} m_Background = {};

static RENDERER *M_GetRenderer(void);
static void M_ReuploadBackground(void);
static void M_ResetPolyList(void);
static void M_SetGLBackend(GFX_GL_BACKEND backend);

static RENDERER *M_GetRenderer(void)
{
    RENDERER *r = nullptr;
    if (g_Config.rendering.render_mode == RM_SOFTWARE) {
        r = &m_Renderer_SW;
    } else if (g_Config.rendering.render_mode == RM_HARDWARE) {
        r = &m_Renderer_HW;
    }
    ASSERT(r != nullptr);
    return r;
}

static void M_ReuploadBackground(void)
{
    // Toggling the bilinear filter causes the texture UVs to be readjusted
    // depending on the player's settings. Because the texture UVs are cached
    // on the GPU by Render_LoadBackgroundFromTexture, they need to be updated
    // whenever the UVs readjust.
    if (m_Background.texture != nullptr) {
        Render_LoadBackgroundFromTexture(
            m_Background.texture, m_Background.repeat_x, m_Background.repeat_y);
    }
}

static void M_ResetPolyList(void)
{
    g_SurfaceCount = 0;
    g_Sort3DPtr = g_SortBuffer;
    g_Info3DPtr = g_Info3DBuffer;

    RENDERER *const r = M_GetRenderer();
    if (r->ResetPolyList != nullptr) {
        r->ResetPolyList(r);
    }
}

static void M_SetGLBackend(const GFX_GL_BACKEND backend)
{
    switch (backend) {
    case GFX_GL_21:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
        break;

    case GFX_GL_33C:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        break;

    case GFX_GL_INVALID_BACKEND:
        ASSERT_FAIL();
        break;
    }
}

void Render_Init(void)
{
    // TODO Move to libtrx later and combine with S_Shell_CreateWindow.
    const GFX_GL_BACKEND backends_to_try[] = {
        // clang-format off
        GFX_GL_33C,
        GFX_GL_21,
        GFX_GL_INVALID_BACKEND, // guard
        // clang-format on
    };

    for (int32_t i = 0; backends_to_try[i] != GFX_GL_INVALID_BACKEND; i++) {
        const GFX_GL_BACKEND backend = backends_to_try[i];

        M_SetGLBackend(backend);

        int32_t major;
        int32_t minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        LOG_DEBUG("Trying GL backend %d.%d", major, minor);
        if (GFX_Context_Attach(g_SDLWindow, backend)) {
            GFX_Context_SetRenderingMode(GFX_RM_FRAMEBUFFER);
            m_FadeRenderer = GFX_FadeRenderer_Create();
            m_BackgroundRenderer = GFX_2D_Renderer_Create();
            Renderer_SW_Prepare(&m_Renderer_SW);
            Renderer_HW_Prepare(&m_Renderer_HW);
            return;
        }
    }

    Shell_ExitSystem("System Error: cannot attach opengl context");
}

void Render_Shutdown(void)
{
    LOG_DEBUG("");
    RENDERER *const r = M_GetRenderer();
    if (r != nullptr) {
        r->Close(r);
        r->Shutdown(r);
    }

    if (m_Background.surface != nullptr) {
        GFX_2D_Surface_Free(m_Background.surface);
        m_Background.surface = nullptr;
    }

    if (m_FadeRenderer != nullptr) {
        GFX_FadeRenderer_Destroy(m_FadeRenderer);
        m_FadeRenderer = nullptr;
    }

    if (m_BackgroundRenderer != nullptr) {
        GFX_2D_Renderer_Destroy(m_BackgroundRenderer);
        m_BackgroundRenderer = nullptr;
    }

    GFX_Context_Detach();
}

void Render_Reset(const RENDER_RESET_FLAGS reset_flags)
{
    LOG_DEBUG("reset_flags=%x", reset_flags);

    if (m_PreviousRenderer != nullptr) {
        m_PreviousRenderer->Close(m_PreviousRenderer);
    }

    RENDERER *const r = M_GetRenderer();
    if (!r->initialized) {
        r->Init(r);
    }
    r->Open(r);
    m_PreviousRenderer = r;

    r->Reset(r, reset_flags);

    if (reset_flags & RENDER_RESET_UVS) {
        Render_ResetTextureUVs();
    }
    if (reset_flags & (RENDER_RESET_PARAMS | RENDER_RESET_TEXTURES)) {
        Render_AdjustTextureUVs(reset_flags & RENDER_RESET_TEXTURES);
        M_ReuploadBackground();
    }
    if (reset_flags & RENDER_RESET_PARAMS) {
        GFX_Context_SetWireframeMode(g_Config.rendering.enable_wireframe);
        GFX_Context_SetLineWidth(g_Config.rendering.wireframe_width);
    }

    LOG_DEBUG("reset finished");
}

void Render_SetupDisplay(
    const int32_t window_border, const int32_t window_width,
    const int32_t window_height, const int32_t screen_width,
    const int32_t screen_height)
{
    LOG_DEBUG("%dx%d", screen_width, screen_height);
    GFX_Context_SetWindowBorder(window_border);
    GFX_Context_SetWindowSize(window_width, window_height);
    GFX_Context_SetDisplaySize(screen_width, screen_height);
    Render_Reset(RENDER_RESET_VIEWPORT);
}

void Render_BeginScene(void)
{
    GFX_Context_Clear();
    RENDERER *const r = M_GetRenderer();
    r->BeginScene(r);
    M_ResetPolyList();
}

void Render_EndScene(void)
{
    RENDERER *const r = M_GetRenderer();
    r->EndScene(r);
    GFX_Context_SwapBuffers();
}

void Render_LoadBackgroundFromTexture(
    const OBJECT_TEXTURE *const texture, const int32_t repeat_x,
    const int32_t repeat_y)
{
    const RGBA_8888 *const page = Output_GetTexturePage32(texture->tex_page);
    if (page == nullptr) {
        return;
    }

    if (m_Background.surface != nullptr) {
        GFX_2D_Surface_Free(m_Background.surface);
        m_Background.surface = nullptr;
    }

    m_Background.ready = true;
    m_Background.texture = texture;
    m_Background.repeat_x = repeat_x;
    m_Background.repeat_y = repeat_y;

    GFX_2D_SURFACE_DESC desc = {
        .width = TEXTURE_PAGE_WIDTH,
        .height = TEXTURE_PAGE_HEIGHT,
        .bit_count = 32,
        .tex_format = GL_RGBA,
        .tex_type = GL_UNSIGNED_INT_8_8_8_8_REV,
        .uv = {
            {
                texture->uv[0].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[0].v / 256.0f / TEXTURE_PAGE_HEIGHT},
            {
                texture->uv[1].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[1].v / 256.0f / TEXTURE_PAGE_HEIGHT},
            {
                texture->uv[2].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[2].v / 256.0f / TEXTURE_PAGE_HEIGHT},
            {
                texture->uv[3].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[3].v / 256.0f / TEXTURE_PAGE_HEIGHT},
        },
        .pitch = TEXTURE_PAGE_WIDTH * 2,
    };
    GFX_2D_Renderer_Upload(m_BackgroundRenderer, &desc, (uint8_t *)page);
    GFX_2D_Renderer_SetRepeat(m_BackgroundRenderer, repeat_x, repeat_y);
    GFX_2D_Renderer_SetEffect(m_BackgroundRenderer, GFX_2D_EFFECT_VIGNETTE);
}

void Render_LoadBackgroundFromImage(const IMAGE *const image)
{
    if (m_Background.surface != nullptr) {
        GFX_2D_Surface_Free(m_Background.surface);
        m_Background.surface = nullptr;
    }
    ASSERT(image != nullptr);
    m_Background.ready = true;
    m_Background.surface = GFX_2D_Surface_CreateFromImage(image);
    m_Background.texture = nullptr;
    m_Background.repeat_x = 1;
    m_Background.repeat_y = 1;
    GFX_2D_Renderer_UploadSurface(m_BackgroundRenderer, m_Background.surface);
    GFX_2D_Renderer_SetRepeat(m_BackgroundRenderer, 1, 1);
    GFX_2D_Renderer_SetEffect(m_BackgroundRenderer, GFX_2D_EFFECT_NONE);
}

void Render_UnloadBackground(void)
{
    if (m_Background.surface != nullptr) {
        GFX_2D_Surface_Free(m_Background.surface);
        m_Background.surface = nullptr;
    }
    m_Background.texture = nullptr;
    m_Background.ready = false;
    GFX_2D_Renderer_SetRepeat(m_BackgroundRenderer, 1, 1);
    GFX_2D_Renderer_SetEffect(m_BackgroundRenderer, GFX_2D_EFFECT_NONE);
}

void Render_DrawBackground(void)
{
    if (!m_Background.ready) {
        return;
    }
    GFX_2D_Renderer_Render(m_BackgroundRenderer);
    Render_EnableZBuffer(true, true);
}

void Render_ClearZBuffer(void)
{
    RENDERER *const r = M_GetRenderer();
    if (r->ClearZBuffer != nullptr) {
        r->ClearZBuffer(r);
    }
}

void Render_EnableZBuffer(const bool z_write_enable, const bool z_test_enable)
{
    RENDERER *const r = M_GetRenderer();
    if (r->EnableZBuffer != nullptr) {
        r->EnableZBuffer(r, z_write_enable, z_test_enable);
    }
}

void Render_DrawPolyList(void)
{
    RENDERER *const r = M_GetRenderer();
    r->DrawPolyList(r);
    M_ResetPolyList();
}

void Render_DrawBlackRectangle(const int32_t opacity)
{
    GFX_FadeRenderer_SetOpacity(m_FadeRenderer, opacity / 255.0f);
    GFX_FadeRenderer_Render(m_FadeRenderer);
}

void Render_InsertFlatFace3s(
    const FACE3 *const faces, const int32_t num, const SORT_TYPE sort_type)
{
    RENDERER *const r = M_GetRenderer();
    if (r->InsertFlatFace3s != nullptr) {
        r->InsertFlatFace3s(r, faces, num, sort_type);
    }
}

void Render_InsertFlatFace4s(
    const FACE4 *const faces, const int32_t num, const SORT_TYPE sort_type)
{
    RENDERER *const r = M_GetRenderer();
    if (r->InsertFlatFace4s != nullptr) {
        r->InsertFlatFace4s(r, faces, num, sort_type);
    }
}

void Render_InsertTexturedFace3s(
    const FACE3 *const faces, const int32_t num, const SORT_TYPE sort_type)
{
    RENDERER *const r = M_GetRenderer();
    if (r->InsertTexturedFace3s != nullptr) {
        r->InsertTexturedFace3s(r, faces, num, sort_type);
    }
}

void Render_InsertTexturedFace4s(
    const FACE4 *const faces, const int32_t num, const SORT_TYPE sort_type)
{
    RENDERER *const r = M_GetRenderer();
    if (r->InsertTexturedFace4s != nullptr) {
        r->InsertTexturedFace4s(r, faces, num, sort_type);
    }
}

void Render_InsertLine(
    const int32_t x0, const int32_t y0, const int32_t x1, const int32_t y1,
    const int32_t z, const uint8_t color_idx)
{
    RENDERER *const r = M_GetRenderer();
    if (r->InsertLine != nullptr) {
        r->InsertLine(r, x0, y0, x1, y1, z, color_idx);
    }
}

void Render_InsertFlatRect(
    const int32_t x0, const int32_t y0, const int32_t x1, const int32_t y1,
    const int32_t z, const uint8_t color_idx)
{
    RENDERER *const r = M_GetRenderer();
    if (r->InsertFlatRect != nullptr) {
        r->InsertFlatRect(r, x0, y0, x1, y1, z, color_idx);
    }
}

void Render_InsertTransQuad(
    const int32_t x, const int32_t y, const int32_t width, const int32_t height,
    const int32_t z)
{
    RENDERER *const r = M_GetRenderer();
    if (r->InsertTransQuad != nullptr) {
        r->InsertTransQuad(r, x, y, width, height, z);
    }
}

void Render_InsertTransOctagon(const PHD_VBUF *const vbuf, const int16_t shade)
{
    RENDERER *const r = M_GetRenderer();
    if (r->InsertTransOctagon != nullptr) {
        r->InsertTransOctagon(r, vbuf, shade);
    }
}

void Render_InsertSprite(
    const int32_t z, const int32_t x0, const int32_t y0, const int32_t x1,
    const int32_t y1, const int32_t sprite_idx, const int16_t shade)
{
    RENDERER *const r = M_GetRenderer();
    if (r->InsertSprite != nullptr) {
        r->InsertSprite(r, z, x0, y0, x1, y1, sprite_idx, shade);
    }
}

void Render_SetWet(const bool is_wet)
{
    RENDERER *const r = M_GetRenderer();
    if (r->SetWet != nullptr) {
        r->SetWet(r, is_wet);
    }
}
