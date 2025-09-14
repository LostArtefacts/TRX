#include "debug.h"
#include "game/objects.h"
#include "game/output/background.h"
#include "game/output/scene_compositor.h"
#include "game/output/scene_source.h"
#include "game/output/textures.h"
#include "gfx/context.h"

#define M_FIT_MODE IMAGE_FIT_SMART

typedef struct {
    SCENE_SOURCE source;
    GFX_2D_RENDERER *renderer;
    GFX_2D_SURFACE *surface;
    BACKGROUND_TYPE type;
    bool staged;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_RenderPass(const SCENE_SOURCE *src, SCENE_PASS pass)
{
    if (pass != SCENE_PASS_BACKGROUND || !m_Priv.staged
        || m_Priv.type == BK_TRANSPARENT) {
        return;
    }
    GFX_2D_Renderer_Render(m_Priv.renderer);
    m_Priv.staged = false;
}

static bool M_IsDirty(const SCENE_SOURCE *src, SCENE_PASS pass)
{
    return pass == SCENE_PASS_BACKGROUND && m_Priv.staged
        && m_Priv.type != BK_TRANSPARENT;
}

void Output_InitBackground(void)
{
    m_Priv.renderer = GFX_2D_Renderer_Create();
    m_Priv.surface = nullptr;
    m_Priv.type = BK_TRANSPARENT;
    m_Priv.staged = false;

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
    if (m_Priv.renderer != nullptr) {
        GFX_2D_Renderer_Destroy(m_Priv.renderer);
        m_Priv.renderer = nullptr;
    }
    Output_ClearLastBackgroundPath();
}

BACKGROUND_TYPE Output_GetBackgroundType(void)
{
    return m_Priv.type;
}

void Output_RefreshBackgroundScaling(void)
{
    if (Output_GetBackgroundType() != BK_IMAGE) {
        GFX_2D_Renderer_SetQuad(m_Priv.renderer, 0.0f, 0.0f, 1.0f, 1.0f);
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

    GFX_2D_Renderer_SetQuad(m_Priv.renderer, x0, y0, x1, y1);
}

bool Output_LoadBackgroundFromImage(const IMAGE *const image)
{
    if (m_Priv.surface != nullptr) {
        GFX_2D_Surface_Free(m_Priv.surface);
    }
    m_Priv.type = BK_IMAGE;
    m_Priv.surface = GFX_2D_Surface_CreateFromImage(image);
    GFX_2D_Renderer_SetRepeat(m_Priv.renderer, 1, 1);
    GFX_2D_Renderer_SetTextureSize(m_Priv.renderer, nullptr);
    GFX_2D_Renderer_Upload(
        m_Priv.renderer, &m_Priv.surface->desc, m_Priv.surface->buffer);
    GFX_2D_Renderer_SetEffect(m_Priv.renderer, GFX_2D_EFFECT_NONE);
    Output_RefreshBackgroundScaling();
    return true;
}

void Output_LoadBackgroundFromObject(const bool wave)
{
#if TR_VERSION == 1
    m_Priv.type = BK_TRANSPARENT;
#else
    if (m_Priv.surface != nullptr) {
        GFX_2D_Surface_Free(m_Priv.surface);
        m_Priv.surface = nullptr;
    }

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
    const int32_t repeat_y = 6;
    const int32_t repeat_x = repeat_y * Viewport_GetWidth(VIEWPORT_GAME)
        / (float)Viewport_GetHeight(VIEWPORT_GAME);
    m_Priv.type = wave ? BK_PATTERN_WAVE : BK_PATTERN_STATIC;

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
    GFX_2D_Renderer_Upload(m_Priv.renderer, &desc, (uint8_t *)page);
    GFX_2D_Renderer_SetRepeat(m_Priv.renderer, repeat_x, repeat_y);
    GFX_2D_Renderer_SetTextureSize(
        m_Priv.renderer,
        &(GFX_TEXTURE_SIZE) {
            .x0 = size.x0,
            .y0 = size.y0,
            .x1 = size.x1,
            .y1 = size.y1,
        });
    GFX_2D_Renderer_SetEffect(
        m_Priv.renderer, wave ? GFX_2D_EFFECT_WAVE : GFX_2D_EFFECT_VIGNETTE);
    Output_RefreshBackgroundScaling();
#endif
}

void Output_UnloadBackground(void)
{
    if (m_Priv.surface != nullptr) {
        GFX_2D_Surface_Free(m_Priv.surface);
        m_Priv.surface = nullptr;
    }
    m_Priv.type = BK_TRANSPARENT;
}

void Output_DrawBackground(void)
{
    if (m_Priv.type != BK_TRANSPARENT) {
        m_Priv.staged = true;
    }
}
