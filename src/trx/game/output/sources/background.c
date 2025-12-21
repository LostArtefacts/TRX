#include <trx/debug.h>
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

    GFX_GL_TEXTURE solid_black_texture;
    float overlay_opacity;
    float dim_opacity;

    uint8_t *readback_geom;
    uint8_t *readback_ui;
    uint8_t *readback_final;
    size_t readback_bytes;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_EnsureReadback(const int32_t w, const int32_t h)
{
    if (w <= 0 || h <= 0) {
        return;
    }

    const size_t bytes = (size_t)w * (size_t)h * 4;
    if (m_Priv.readback_bytes == bytes) {
        return;
    }

    m_Priv.readback_geom = Memory_Realloc(m_Priv.readback_geom, bytes);
    m_Priv.readback_ui = Memory_Realloc(m_Priv.readback_ui, bytes);
    m_Priv.readback_final = Memory_Realloc(m_Priv.readback_final, bytes);
    m_Priv.readback_bytes = bytes;
}

static void M_EnsureSnapshotTexture(void)
{
    const int32_t w = Viewport_GetWidth(VIEWPORT_GAME);
    const int32_t h = Viewport_GetHeight(VIEWPORT_GAME);
    if (w <= 0 || h <= 0) {
        return;
    }

    if (m_Priv.snapshot_texture.initialized && m_Priv.snapshot_width == w
        && m_Priv.snapshot_height == h) {
        return;
    }

    if (!m_Priv.snapshot_texture.initialized) {
        GFX_GL_Texture_Init(&m_Priv.snapshot_texture, GL_TEXTURE_2D);
    }

    m_Priv.snapshot_width = w;
    m_Priv.snapshot_height = h;

    GFX_GL_Texture_Bind(&m_Priv.snapshot_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        nullptr);
    GFX_GL_CheckError();
}

static void M_EnsureSolidBlackTexture(void)
{
    if (m_Priv.solid_black_texture.initialized) {
        return;
    }
    GFX_GL_Texture_Init(&m_Priv.solid_black_texture, GL_TEXTURE_2D);
    const uint8_t pixel[4] = { 0, 0, 0, 255 };
    GFX_GL_Texture_Bind(&m_Priv.solid_black_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        &pixel[0]);
    GFX_GL_CheckError();
}

static void M_RenderPass(const SCENE_SOURCE *src, SCENE_PASS pass)
{
    if (pass != SCENE_PASS_BACKGROUND || !m_Priv.staged) {
        return;
    }

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

    m_Priv.staged = false;
}

static bool M_IsDirty(const SCENE_SOURCE *src, SCENE_PASS pass)
{
    return pass == SCENE_PASS_BACKGROUND && m_Priv.staged;
}

void Output_InitBackground(void)
{
    m_Priv.snapshot_renderer = GFX_2D_Renderer_Create();
    m_Priv.overlay_renderer = GFX_2D_Renderer_Create();
    m_Priv.surface = nullptr;
    m_Priv.type = BK_TRANSPARENT;
    m_Priv.staged = false;
    m_Priv.snapshot_enabled = false;
    m_Priv.snapshot_width = 0;
    m_Priv.snapshot_height = 0;
    m_Priv.snapshot_desaturation = 0.0f;
    m_Priv.overlay_opacity = 1.0f;
    m_Priv.dim_opacity = 0.0f;
    m_Priv.snapshot_texture = (GFX_GL_TEXTURE) { .initialized = false };
    m_Priv.solid_black_texture = (GFX_GL_TEXTURE) { .initialized = false };
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
    if (m_Priv.snapshot_texture.initialized) {
        GFX_GL_Texture_Close(&m_Priv.snapshot_texture);
        m_Priv.snapshot_texture.initialized = false;
    }
    if (m_Priv.solid_black_texture.initialized) {
        GFX_GL_Texture_Close(&m_Priv.solid_black_texture);
        m_Priv.solid_black_texture.initialized = false;
    }
    Memory_FreePointer(&m_Priv.readback_geom);
    Memory_FreePointer(&m_Priv.readback_ui);
    Memory_FreePointer(&m_Priv.readback_final);
    m_Priv.readback_bytes = 0;
    Output_ClearLastBackgroundPath();
}

BACKGROUND_TYPE Output_GetBackgroundType(void)
{
    return m_Priv.type;
}

void Output_RefreshBackgroundScaling(void)
{
    if (Output_GetBackgroundType() != BK_IMAGE) {
        GFX_2D_Renderer_SetQuad(
            m_Priv.overlay_renderer, 0.0f, 0.0f, 1.0f, 1.0f);
        return;
    }

    const float src_w = m_Priv.surface->desc.width;
    const float src_h = m_Priv.surface->desc.height;
    const float dst_w = Viewport_GetWidth(VIEWPORT_GAME);
    const float dst_h = Viewport_GetHeight(VIEWPORT_GAME);
    const float src_ratio = src_w / src_h;
    const float dst_ratio = dst_w / dst_h;

    IMAGE_FIT_MODE fit_mode = M_FIT_MODE;
    if (fit_mode == IMAGE_FIT_SMART) {
        const float ar_diff = (src_ratio > dst_ratio ? src_ratio / dst_ratio
                                                     : dst_ratio / src_ratio)
            - 1.0f;
        if (ar_diff <= 0.1f) {
            // if the difference between aspect ratios is under 10%, just
            // stretch it
            fit_mode = IMAGE_FIT_STRETCH;
        } else if (src_ratio <= dst_ratio) {
            // if the viewport is too wide, center the image
            fit_mode = IMAGE_FIT_LETTERBOX;
        } else {
            // if the image is too wide, crop the image
            fit_mode = IMAGE_FIT_CROP;
        }
    }

    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 1.0f;
    float y1 = 1.0f;

    switch (fit_mode) {
    case IMAGE_FIT_STRETCH:
        break;

    case IMAGE_FIT_CROP:
        if (src_ratio < dst_ratio) {
            const float height = dst_ratio / src_ratio;
            y0 = (1.0f - height) * 0.5f;
            y1 = y0 + height;
        } else {
            const float width = src_ratio / dst_ratio;
            x0 = (1.0f - width) * 0.5f;
            x1 = x0 + width;
        }
        break;

    case IMAGE_FIT_LETTERBOX:
        if (src_ratio > dst_ratio) {
            float height = dst_ratio / src_ratio;
            y0 = (1.0f - height) * 0.5f;
            y1 = y0 + height;
        } else {
            float width = src_ratio / dst_ratio;
            x0 = (1.0f - width) * 0.5f;
            x1 = x0 + width;
        }
        break;

    default:
        ASSERT_FAIL();
        break;
    }

    GFX_2D_Renderer_SetQuad(m_Priv.overlay_renderer, x0, y0, x1, y1);
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
}

void Output_DrawBackground(void)
{
    if (m_Priv.type != BK_TRANSPARENT || m_Priv.snapshot_enabled
        || m_Priv.dim_opacity > 0.0f) {
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
}

bool Output_Background_IsSnapshotEnabled(void)
{
    return m_Priv.snapshot_enabled;
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

    GFX_GL_Texture_Bind(&m_Priv.snapshot_texture);
    glCopyTexSubImage2D(
        GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_Priv.snapshot_width,
        m_Priv.snapshot_height);
    GFX_GL_CheckError();
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
