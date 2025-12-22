#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/fader.h>
#include <trx/game/objects.h>
#include <trx/game/output/background.h>
#include <trx/game/output/scene_compositor.h>
#include <trx/game/output/scene_source.h>
#include <trx/game/output/textures.h>
#include <trx/gfx/context.h>
#include <trx/gfx/gl/texture.h>
#include <trx/gfx/gl/utils.h>
#include <trx/gfx/renderer.h>
#include <trx/memory.h>
#include <trx/utils.h>
#include <trx/version.h>

#define M_FIT_MODE IMAGE_FIT_SMART

typedef struct {
    SCENE_SOURCE source;
    GFX_2D_RENDERER *snapshot_renderer;
    GFX_2D_RENDERER *overlay_renderer;
    GFX_2D_SURFACE *surface;
    BACKGROUND_TYPE type;
    bool staged;

    bool snapshot_enabled;
    GFX_GL_TEXTURE snapshot_texture;
    int32_t snapshot_width;
    int32_t snapshot_height;
    float snapshot_desaturation;
    bool snapshot_has_content;
    float snapshot_brightness;

    bool transition_texture_valid;
    GFX_GL_TEXTURE transition_texture;
    int32_t transition_width;
    int32_t transition_height;

    GFX_GL_TEXTURE solid_black_texture;
    float overlay_opacity;
    float dim_opacity;

    bool transition_active;
    FADER transition_fader;
} M_PRIV;

static M_PRIV m_Priv = {};

static bool M_CreateTextureRGBA8(
    GFX_GL_TEXTURE *const texture, const int32_t width, const int32_t height,
    const void *const data)
{
    if (texture == nullptr || width <= 0 || height <= 0) {
        return false;
    }
    if (!texture->initialized) {
        GFX_GL_Texture_Init(texture, GL_TEXTURE_2D);
    }
    GFX_GL_Texture_Bind(texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        data);
    GFX_GL_CheckError();
    return true;
}

static void M_CloseTexture(GFX_GL_TEXTURE *const texture)
{
    if (texture != nullptr && texture->initialized) {
        GFX_GL_Texture_Close(texture);
        texture->initialized = false;
    }
}

static void M_ResizeSnapshotTexture(const int32_t width, const int32_t height)
{
    if (!m_Priv.snapshot_texture.initialized || width <= 0 || height <= 0
        || m_Priv.snapshot_width <= 0 || m_Priv.snapshot_height <= 0) {
        return;
    }

    GFX_GL_TEXTURE resized_texture = { .initialized = false };
    if (!M_CreateTextureRGBA8(&resized_texture, width, height, nullptr)) {
        return;
    }

    GLuint read_fbo = 0;
    GLuint draw_fbo = 0;
    glGenFramebuffers(1, &read_fbo);
    glGenFramebuffers(1, &draw_fbo);

    GLint prev_read_fbo = 0;
    GLint prev_draw_fbo = 0;
    GLint prev_read_buffer = 0;
    GLint prev_draw_buffer = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_draw_fbo);
    glGetIntegerv(GL_READ_BUFFER, &prev_read_buffer);
    glGetIntegerv(GL_DRAW_BUFFER, &prev_draw_buffer);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo);
    glFramebufferTexture2D(
        GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        m_Priv.snapshot_texture.id, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fbo);
    glFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        resized_texture.id, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBlitFramebuffer(
        0, 0, m_Priv.snapshot_width, m_Priv.snapshot_height, 0, 0, width,
        height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)prev_read_fbo);
    glReadBuffer(prev_read_buffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)prev_draw_fbo);
    glDrawBuffer(prev_draw_buffer);
    glDeleteFramebuffers(1, &read_fbo);
    glDeleteFramebuffers(1, &draw_fbo);

    GFX_GL_Texture_Close(&m_Priv.snapshot_texture);
    m_Priv.snapshot_texture = resized_texture;
    m_Priv.snapshot_width = width;
    m_Priv.snapshot_height = height;
    GFX_GL_CheckError();
}

static void M_EnsureSnapshotTexture(void)
{
    const int32_t w = Viewport_GetWidth(VIEWPORT_GAME);
    const int32_t h = Viewport_GetHeight(VIEWPORT_GAME);
    if (w <= 0 || h <= 0) {
        return;
    }

    // Keep existing contents during live screens (e.g. inventory/stats) even
    // if the viewport changes, to avoid wiping the snapshot on resize.
    if (m_Priv.snapshot_texture.initialized && m_Priv.snapshot_has_content) {
        if (m_Priv.snapshot_width != w || m_Priv.snapshot_height != h) {
            M_ResizeSnapshotTexture(w, h);
        }
        return;
    }

    m_Priv.snapshot_width = w;
    m_Priv.snapshot_height = h;

    if (!M_CreateTextureRGBA8(
            &m_Priv.snapshot_texture, m_Priv.snapshot_width,
            m_Priv.snapshot_height, nullptr)) {
        return;
    }
    m_Priv.transition_texture_valid = false;
}

static void M_EnsureTransitionTexture(void)
{
    const int32_t w = Viewport_GetWidth(VIEWPORT_GAME);
    const int32_t h = Viewport_GetHeight(VIEWPORT_GAME);
    if (w <= 0 || h <= 0) {
        return;
    }

    if (m_Priv.transition_texture.initialized && m_Priv.transition_width == w
        && m_Priv.transition_height == h) {
        return;
    }

    m_Priv.transition_width = w;
    m_Priv.transition_height = h;

    M_CreateTextureRGBA8(
        &m_Priv.transition_texture, m_Priv.transition_width,
        m_Priv.transition_height, nullptr);
}

static void M_EnsureSolidBlackTexture(void)
{
    if (m_Priv.solid_black_texture.initialized) {
        return;
    }
    const uint8_t pixel[4] = { 0, 0, 0, 255 };
    M_CreateTextureRGBA8(&m_Priv.solid_black_texture, 1, 1, &pixel[0]);
}

static void M_RenderPassBackground(void)
{
    if (m_Priv.snapshot_enabled) {
        M_EnsureSnapshotTexture();

        GFX_2D_Renderer_SetExternalTexture(
            m_Priv.snapshot_renderer, m_Priv.snapshot_texture.id,
            m_Priv.snapshot_width, m_Priv.snapshot_height, true);
        GFX_2D_Renderer_SetEffect(m_Priv.snapshot_renderer, GFX_2D_EFFECT_NONE);
        GFX_2D_Renderer_SetRepeat(m_Priv.snapshot_renderer, 1, 1);
        GFX_2D_Renderer_SetTextureSize(m_Priv.snapshot_renderer, nullptr);
        GFX_2D_Renderer_SetOpacity(m_Priv.snapshot_renderer, 1.0f);
        GFX_2D_Renderer_SetDesaturation(
            m_Priv.snapshot_renderer, m_Priv.snapshot_desaturation);
        GFX_2D_Renderer_Render(m_Priv.snapshot_renderer);
    }

    const bool use_blend =
        m_Priv.snapshot_enabled || m_Priv.overlay_opacity < 1.0f;
    if (m_Priv.type != BK_TRANSPARENT && m_Priv.type != BK_MONOCHROME) {
        GFX_2D_Renderer_SetOpacity(
            m_Priv.overlay_renderer, m_Priv.overlay_opacity);
        GFX_2D_Renderer_SetDesaturation(m_Priv.overlay_renderer, 0.0f);
        if (use_blend) {
            GFX_2D_Renderer_RenderWithBlend(m_Priv.overlay_renderer);
        } else {
            GFX_2D_Renderer_Render(m_Priv.overlay_renderer);
        }
    }

    if (m_Priv.dim_opacity > 0.0f) {
        M_EnsureSolidBlackTexture();
        GFX_2D_Renderer_SetExternalTexture(
            m_Priv.snapshot_renderer, m_Priv.solid_black_texture.id, 1, 1,
            false);
        GFX_2D_Renderer_SetEffect(m_Priv.snapshot_renderer, GFX_2D_EFFECT_NONE);
        GFX_2D_Renderer_SetRepeat(m_Priv.snapshot_renderer, 1, 1);
        GFX_2D_Renderer_SetTextureSize(m_Priv.snapshot_renderer, nullptr);
        GFX_2D_Renderer_SetOpacity(
            m_Priv.snapshot_renderer, m_Priv.dim_opacity);
        GFX_2D_Renderer_SetDesaturation(m_Priv.snapshot_renderer, 0.0f);
        GFX_2D_Renderer_RenderWithBlend(m_Priv.snapshot_renderer);
    }
}

static void M_RenderPassTransition(void)
{
    const int32_t current = Fader_GetRealValue(&m_Priv.transition_fader);
    m_Priv.transition_fader.target_drawn |=
        current == m_Priv.transition_fader.args.target;
    const float opacity = current / 255.0f;

    if (opacity <= 0.0f || !m_Priv.transition_texture.initialized
        || !m_Priv.transition_texture_valid) {
        m_Priv.transition_active = false;
        m_Priv.transition_texture_valid = false;
        return;
    }

    GFX_2D_Renderer_SetExternalTexture(
        m_Priv.snapshot_renderer, m_Priv.transition_texture.id,
        m_Priv.transition_width, m_Priv.transition_height, true);
    GFX_2D_Renderer_SetEffect(m_Priv.snapshot_renderer, GFX_2D_EFFECT_NONE);
    GFX_2D_Renderer_SetRepeat(m_Priv.snapshot_renderer, 1, 1);
    GFX_2D_Renderer_SetTextureSize(m_Priv.snapshot_renderer, nullptr);
    GFX_2D_Renderer_SetOpacity(m_Priv.snapshot_renderer, opacity);
    GFX_2D_Renderer_SetDesaturation(m_Priv.snapshot_renderer, 0.0f);
    GFX_2D_Renderer_RenderWithBlend(m_Priv.snapshot_renderer);

    if (!Fader_IsActive(&m_Priv.transition_fader)) {
        m_Priv.transition_active = false;
        m_Priv.transition_texture_valid = false;
    }
}

static void M_RenderPass(const SCENE_SOURCE *const src, const SCENE_PASS pass)
{
    if (pass != SCENE_PASS_BACKGROUND) {
        return;
    }

    if (m_Priv.staged) {
        M_RenderPassBackground();
        m_Priv.staged = false;
    }

    if (m_Priv.transition_active) {
        M_RenderPassTransition();
    }
}

static bool M_IsDirty(const SCENE_SOURCE *const src, const SCENE_PASS pass)
{
    if (pass == SCENE_PASS_BACKGROUND) {
        return m_Priv.staged || m_Priv.transition_active;
    }
    return false;
}

void Output_InitBackground(void)
{
    m_Priv = (M_PRIV) {
        .type = BK_TRANSPARENT,
        .snapshot_brightness = 1.0f,
        .overlay_opacity = 1.0f,
    };
    m_Priv.snapshot_renderer = GFX_2D_Renderer_Create();
    m_Priv.overlay_renderer = GFX_2D_Renderer_Create();
    M_EnsureSolidBlackTexture();

    m_Priv.source.render_pass = M_RenderPass;
    m_Priv.source.is_dirty = M_IsDirty;
    SceneCompositor_AddSource(&m_Priv.source);
}

void Output_ShutdownBackground(void)
{
    if (m_Priv.surface != nullptr) {
        GFX_2D_Surface_Free(m_Priv.surface);
        m_Priv.surface = nullptr;
    }
    if (m_Priv.snapshot_renderer != nullptr) {
        GFX_2D_Renderer_Destroy(m_Priv.snapshot_renderer);
        m_Priv.snapshot_renderer = nullptr;
    }
    if (m_Priv.overlay_renderer != nullptr) {
        GFX_2D_Renderer_Destroy(m_Priv.overlay_renderer);
        m_Priv.overlay_renderer = nullptr;
    }
    M_CloseTexture(&m_Priv.snapshot_texture);
    M_CloseTexture(&m_Priv.transition_texture);
    m_Priv.transition_texture_valid = false;
    M_CloseTexture(&m_Priv.solid_black_texture);
    Output_ClearLastBackgroundPath();
}

BACKGROUND_TYPE Output_GetBackgroundType(void)
{
    return m_Priv.type;
}

void Output_RefreshBackgroundScaling(void)
{
    if (Output_GetBackgroundType() != BK_IMAGE) {
        GFX_2D_Renderer_ClearFit(m_Priv.overlay_renderer);
        return;
    }

    if (m_Priv.surface == nullptr) {
        GFX_2D_Renderer_ClearFit(m_Priv.overlay_renderer);
        return;
    }

    const float src_w = (float)m_Priv.surface->desc.width;
    const float src_h = (float)m_Priv.surface->desc.height;

    GFX_2D_FIT_MODE fit_mode = GFX_2D_FIT_SMART;
    switch (M_FIT_MODE) {
    case IMAGE_FIT_STRETCH:
        fit_mode = GFX_2D_FIT_STRETCH;
        break;
    case IMAGE_FIT_LETTERBOX:
        fit_mode = GFX_2D_FIT_LETTERBOX;
        break;
    case IMAGE_FIT_CROP:
        fit_mode = GFX_2D_FIT_CROP;
        break;
    case IMAGE_FIT_SMART:
        fit_mode = GFX_2D_FIT_SMART;
        break;
    default:
        ASSERT_FAIL();
        break;
    }

    GFX_2D_Renderer_SetFit(m_Priv.overlay_renderer, fit_mode, src_w, src_h);
}

bool Output_LoadBackgroundFromImage(const IMAGE *const image)
{
    if (m_Priv.surface != nullptr) {
        GFX_2D_Surface_Free(m_Priv.surface);
    }
    m_Priv.type = BK_IMAGE;
    m_Priv.surface = GFX_2D_Surface_CreateFromImage(image);
    m_Priv.overlay_opacity = 1.0f;
    GFX_2D_Renderer_SetRepeat(m_Priv.overlay_renderer, 1, 1);
    GFX_2D_Renderer_SetTextureSize(m_Priv.overlay_renderer, nullptr);
    GFX_2D_Renderer_Upload(
        m_Priv.overlay_renderer, &m_Priv.surface->desc, m_Priv.surface->buffer);
    GFX_2D_Renderer_SetEffect(m_Priv.overlay_renderer, GFX_2D_EFFECT_NONE);
    Output_RefreshBackgroundScaling();
    return true;
}

void Output_LoadBackgroundFromObject(const bool wave)
{
    if (m_Priv.surface != nullptr) {
        GFX_2D_Surface_Free(m_Priv.surface);
        m_Priv.surface = nullptr;
    }

    const OBJECT *const obj = Object_Get(O_INV_BACKGROUND);
    if (!obj->loaded) {
        return;
    }

    const OBJECT_MESH *const mesh = Object_GetMesh(obj->mesh_idx);
    if (mesh->tex_face4s.count < 1) {
        return;
    }

    const int32_t texture_idx = mesh->tex_face4s.data[0].texture_idx;
    const OBJECT_TEXTURE *const texture = Output_GetObjectTexture(texture_idx);
    ASSERT(texture != nullptr);
    const int32_t repeat_y = 6;
    const int32_t repeat_x = repeat_y * Viewport_GetWidth(VIEWPORT_GAME)
        / (float)Viewport_GetHeight(VIEWPORT_GAME);
    m_Priv.type = wave ? BK_PATTERN_WAVE : BK_PATTERN_STATIC;
    m_Priv.overlay_opacity = 1.0f;

    const RGBA_8888 *const page = Output_GetTexturePage32(texture->tex_page);
    if (page == nullptr) {
        return;
    }

    GFX_2D_SURFACE_DESC desc = {
        .width = TEXTURE_PAGE_WIDTH,
        .height = TEXTURE_PAGE_HEIGHT,
        .bit_count = 32,
        .tex_format = GL_RGBA,
        .tex_type = GL_UNSIGNED_INT_8_8_8_8_REV,
        .uv = {
            {
                texture->uv[0].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[0].v / 256.0f / TEXTURE_PAGE_HEIGHT
            },
            {
                texture->uv[1].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[1].v / 256.0f / TEXTURE_PAGE_HEIGHT
            },
            {
                texture->uv[2].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[2].v / 256.0f / TEXTURE_PAGE_HEIGHT
            },
            {
                texture->uv[3].u / 256.0f / TEXTURE_PAGE_WIDTH,
                texture->uv[3].v / 256.0f / TEXTURE_PAGE_HEIGHT
            },
        },
        .pitch = TEXTURE_PAGE_WIDTH * 2,
    };
    const OUTPUT_TEXTURE_SIZE size = Output_Textures_GetAtlasSize(texture_idx);
    GFX_2D_Renderer_Upload(m_Priv.overlay_renderer, &desc, (uint8_t *)page);
    GFX_2D_Renderer_SetRepeat(m_Priv.overlay_renderer, repeat_x, repeat_y);
    GFX_2D_Renderer_SetTextureSize(
        m_Priv.overlay_renderer,
        &(GFX_TEXTURE_SIZE) {
            .x0 = size.x0,
            .y0 = size.y0,
            .x1 = size.x1,
            .y1 = size.y1,
        });
    GFX_2D_Renderer_SetEffect(
        m_Priv.overlay_renderer,
        wave ? GFX_2D_EFFECT_WAVE : GFX_2D_EFFECT_VIGNETTE);
    Output_RefreshBackgroundScaling();
}

void Output_UnloadBackground(void)
{
    if (m_Priv.surface != nullptr) {
        GFX_2D_Surface_Free(m_Priv.surface);
        m_Priv.surface = nullptr;
    }
    m_Priv.type = BK_TRANSPARENT;
    m_Priv.overlay_opacity = 1.0f;
    m_Priv.dim_opacity = 0.0f;
    m_Priv.snapshot_has_content = false;
    GFX_2D_Renderer_ClearFit(m_Priv.overlay_renderer);
}

void Output_DrawBackground(void)
{
    if (m_Priv.type != BK_TRANSPARENT || m_Priv.snapshot_enabled
        || m_Priv.dim_opacity > 0.0f || m_Priv.transition_active) {
        m_Priv.staged = true;
    }
}

void Output_Background_LoadMono(void)
{
    Output_UnloadBackground();
    m_Priv.type = BK_MONOCHROME;
}

void Output_Background_EnableSnapshot(const bool enable)
{
    m_Priv.snapshot_enabled = enable;
    if (!enable) {
        m_Priv.snapshot_has_content = false;
        GFX_2D_Renderer_SetBrightnessScale(m_Priv.snapshot_renderer, 1.0f);
    }
}

bool Output_Background_IsSnapshotEnabled(void)
{
    return m_Priv.snapshot_enabled;
}

static bool M_PrepareViewportCopy(
    const VIEWPORT_SPACE viewport, const int32_t desired_width,
    const int32_t desired_height, VIEWPORT_RECT *const rect,
    int32_t *const copy_width, int32_t *const copy_height)
{
    if (rect == nullptr || copy_width == nullptr || copy_height == nullptr) {
        return false;
    }

    const VIEWPORT_RECT viewport_rect = Viewport_GetRect(viewport);
    if (viewport_rect.width <= 0 || viewport_rect.height <= 0) {
        return false;
    }

    *rect = viewport_rect;
    *copy_width = desired_width;
    *copy_height = desired_height;
    CLAMPG(*copy_width, viewport_rect.width);
    CLAMPG(*copy_height, viewport_rect.height);
    return *copy_width > 0 && *copy_height > 0;
}

static void M_CopyPresentedFrameToTexture(
    GFX_GL_TEXTURE *const texture, const int32_t width, const int32_t height)
{
    if (texture == nullptr || !texture->initialized || width <= 0
        || height <= 0) {
        return;
    }

    VIEWPORT_RECT rect;
    int32_t copy_width = 0;
    int32_t copy_height = 0;
    if (!M_PrepareViewportCopy(
            VIEWPORT_TARGET, width, height, &rect, &copy_width, &copy_height)) {
        return;
    }

    GLint prev_read_fbo = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
    GLint prev_read_buffer = 0;
    glGetIntegerv(GL_READ_BUFFER, &prev_read_buffer);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_FRONT);

    GFX_GL_Texture_Bind(texture);
    glCopyTexSubImage2D(
        GL_TEXTURE_2D, 0, 0, 0, rect.x, rect.y, copy_width, copy_height);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)prev_read_fbo);
    glReadBuffer(prev_read_buffer);
    GFX_GL_CheckError();
}

void Output_Background_CaptureSnapshotScene(void)
{
    if (!m_Priv.snapshot_enabled) {
        return;
    }

    M_EnsureSnapshotTexture();
    if (!m_Priv.snapshot_texture.initialized) {
        return;
    }

    GFX_Renderer_BindGeometryFbo();
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    VIEWPORT_RECT rect;
    int32_t copy_width = 0;
    int32_t copy_height = 0;
    if (!M_PrepareViewportCopy(
            VIEWPORT_GAME, m_Priv.snapshot_width, m_Priv.snapshot_height, &rect,
            &copy_width, &copy_height)) {
        return;
    }

    GFX_GL_Texture_Bind(&m_Priv.snapshot_texture);
    glCopyTexSubImage2D(
        GL_TEXTURE_2D, 0, 0, 0, rect.x, rect.y, copy_width, copy_height);
    GFX_GL_CheckError();
    m_Priv.snapshot_has_content = true;

    // Remove the captured brightness so we can reapply the current multiplier.
    m_Priv.snapshot_brightness = g_Config.visuals.brightness;
    CLAMPL(m_Priv.snapshot_brightness, 0.001f);
    GFX_2D_Renderer_SetBrightnessScale(
        m_Priv.snapshot_renderer, 1.0f / m_Priv.snapshot_brightness);
}

void Output_Background_CaptureSnapshotPresented(void)
{
    if (!m_Priv.snapshot_enabled) {
        return;
    }

    M_EnsureSnapshotTexture();
    if (!m_Priv.snapshot_texture.initialized) {
        return;
    }

    M_CopyPresentedFrameToTexture(
        &m_Priv.snapshot_texture, m_Priv.snapshot_width,
        m_Priv.snapshot_height);
}

void Output_Background_BeginTransitionFadeOut(const double duration)
{
    M_EnsureTransitionTexture();
    if (!m_Priv.transition_texture.initialized) {
        return;
    }

    m_Priv.transition_active = true;
    m_Priv.transition_texture_valid = true;
    M_CopyPresentedFrameToTexture(
        &m_Priv.transition_texture, m_Priv.transition_width,
        m_Priv.transition_height);
    Fader_Init(
        &m_Priv.transition_fader, FADER_BLACK, FADER_TRANSPARENT, duration);
}

void Output_Background_SetOverlayOpacity(float opacity)
{
    CLAMP(opacity, 0.0f, 1.0f);

    if (m_Priv.type == BK_TRANSPARENT) {
        m_Priv.dim_opacity = opacity;
    } else if (m_Priv.type == BK_MONOCHROME) {
        m_Priv.snapshot_desaturation = opacity;
    } else {
        m_Priv.overlay_opacity = opacity;
    }
    m_Priv.staged = true;
}
