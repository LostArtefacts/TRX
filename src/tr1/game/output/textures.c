#include "game/output/textures.h"

#include "game/output.h"
#include "game/output/vertex_range.h"

#include <libtrx/debug.h>
#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>

#include <stdlib.h>

typedef struct {
    OUTPUT_UVW corners[4];
} M_UVW_PACK;

static struct {
    VECTOR *objects;
    VECTOR *sprites;
} m_AnimationRanges;

static struct {
    GLuint tex_atlas;
    GLuint tex_env_map;

    struct {
        int32_t count;
        int32_t count_objects;
        int32_t count_sprites;
        M_UVW_PACK *data;
        M_UVW_PACK *data_objects;
        M_UVW_PACK *data_sprites;

        bool *animated_objects;
        bool *animated_sprites;
    } uvws;

    struct {
        OUTPUT_TEXTURE_SIZE *data;
        OUTPUT_TEXTURE_SIZE *data_objects;
        OUTPUT_TEXTURE_SIZE *data_sprites;
    } atlas_sizes;
} m_Priv = {};

static void M_PrepareObjectAnimationRanges(void)
{
    size_t required_size = 0;
    for (const ANIMATED_TEXTURE_RANGE *src_range =
             Output_GetAnimatedTextureRange(0);
         src_range != nullptr; src_range = src_range->next_range) {
        required_size += src_range->num_textures;
    }

    Vector_Clear(m_AnimationRanges.objects);
    Vector_EnsureCapacity(m_AnimationRanges.objects, required_size);

    for (const ANIMATED_TEXTURE_RANGE *src_range =
             Output_GetAnimatedTextureRange(0);
         src_range != nullptr; src_range = src_range->next_range) {
        for (int32_t i = 0; i < src_range->num_textures; i++) {
            Vector_Add(
                m_AnimationRanges.objects,
                &(OUTPUT_VERTEX_RANGE) {
                    .vertex_start = src_range->textures[i],
                    .vertex_count = 1,
                });
        }
    }
    Output_GlueVertexRanges(m_AnimationRanges.objects);
}

static void M_PrepareSpriteAnimationRanges(void)
{
    size_t required_size = 0;
    for (int32_t i = 0; i < MAX_STATIC_OBJECTS; i++) {
        const STATIC_OBJECT_2D *const obj = Object_Get2DStatic(i);
        if (!obj->loaded || obj->frame_count == 1) {
            continue;
        }
        required_size++;
    }

    Vector_Clear(m_AnimationRanges.sprites);
    Vector_EnsureCapacity(m_AnimationRanges.sprites, required_size);

    for (int32_t i = 0; i < MAX_STATIC_OBJECTS; i++) {
        const STATIC_OBJECT_2D *const obj = Object_Get2DStatic(i);
        if (!obj->loaded || obj->frame_count == 1) {
            continue;
        }
        Vector_Add(
            m_AnimationRanges.sprites,
            &(OUTPUT_VERTEX_RANGE) {
                .vertex_start = obj->texture_idx,
                .vertex_count = obj->frame_count,
            });
    }
    Output_GlueVertexRanges(m_AnimationRanges.sprites);
}

static void M_PrepareAnimationRanges(void)
{
    M_PrepareObjectAnimationRanges();
    M_PrepareSpriteAnimationRanges();

    for (int32_t i = 0; i < Output_GetObjectTextureCount(); i++) {
        m_Priv.uvws.animated_objects[i] = false;
        for (int32_t j = 0; j < m_AnimationRanges.objects->count; j++) {
            const OUTPUT_VERTEX_RANGE *const dst_range =
                Vector_Get(m_AnimationRanges.objects, j);
            const int32_t range_start = dst_range->vertex_start;
            const int32_t range_end = range_start + dst_range->vertex_count;
            if (i >= range_start && i < range_end) {
                m_Priv.uvws.animated_objects[i] = true;
                break;
            }
        }
    }

    for (int32_t i = 0; i < Output_GetSpriteTextureCount(); i++) {
        m_Priv.uvws.animated_sprites[i] = false;
        for (int32_t j = 0; j < m_AnimationRanges.sprites->count; j++) {
            const OUTPUT_VERTEX_RANGE *const dst_range =
                Vector_Get(m_AnimationRanges.sprites, j);
            const int32_t range_start = dst_range->vertex_start;
            const int32_t range_end = range_start + dst_range->vertex_count;
            if (i >= range_start && i < range_end) {
                m_Priv.uvws.animated_sprites[i] = true;
                break;
            }
        }
    }
}

static void M_FillAtlasObjectSize(const int32_t i)
{
    OUTPUT_TEXTURE_SIZE *const size = &m_Priv.atlas_sizes.data_objects[i];
    const OBJECT_TEXTURE *const texture = Output_GetObjectTexture(i);
    size->x0 = texture->uv[0].u;
    size->y0 = texture->uv[0].v;
    size->x1 = texture->uv[0].u;
    size->y1 = texture->uv[0].v;
    for (int32_t j = 1; j < 3; j++) {
        size->x0 = MIN(size->x0, texture->uv[j].u);
        size->y0 = MIN(size->y0, texture->uv[j].v);
        size->x1 = MAX(size->x1, texture->uv[j].u);
        size->y1 = MAX(size->y1, texture->uv[j].v);
    }
    size->x0 /= 65535.0;
    size->y0 /= 65535.0;
    size->x1 /= 65535.0;
    size->y1 /= 65535.0;
}

static void M_FillAtlasSpriteSize(const int32_t i)
{
    OUTPUT_TEXTURE_SIZE *const size = &m_Priv.atlas_sizes.data_sprites[i];
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(i);
    const float adj = 0.1 / 256.0f;
    const float u0 = (sprite->offset & 0xFF) / 256.0f + adj;
    const float v0 = (sprite->offset >> 8) / 256.0f + adj;
    const float u1 = u0 + sprite->width / 65536.0f - 2 * adj;
    const float v1 = v0 + sprite->height / 65536.0f - 2 * adj;
    size->x0 = u0;
    size->y0 = v0;
    size->x1 = u1;
    size->y1 = v1;
}

static void M_FillObjectUVW(const int32_t i)
{
    const OBJECT_TEXTURE *const texture = Output_GetObjectTexture(i);
    OUTPUT_UVW *const corners = m_Priv.uvws.data_objects[i].corners;
    for (int32_t j = 0; j < 4; j++) {
        corners[j].u = texture->uv[j].u / 65535.0f;
        corners[j].v = texture->uv[j].v / 65535.0f;
        corners[j].w = texture->tex_page;
    }
}

static void M_FillSpriteUVW(const int32_t i)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(i);
    const float adj = 0.1 / 256.0f;
    const float u0 = (sprite->offset & 0xFF) / 256.0f + adj;
    const float v0 = (sprite->offset >> 8) / 256.0f + adj;
    const float u1 = u0 + sprite->width / 65536.0f - 2 * adj;
    const float v1 = v0 + sprite->height / 65536.0f - 2 * adj;
    OUTPUT_UVW *const corners = m_Priv.uvws.data_sprites[i].corners;
    // clang-format off
    corners[0].u = u0; corners[0].v = v0; corners[0].w = sprite->tex_page;
    corners[1].u = u1; corners[1].v = v0; corners[1].w = sprite->tex_page;
    corners[2].u = u1; corners[2].v = v1; corners[2].w = sprite->tex_page;
    corners[3].u = u0; corners[3].v = v1; corners[3].w = sprite->tex_page;
    // clang-format on
}

static void M_FillObjectUVWs(void)
{
    for (int32_t i = 0; i < Output_GetObjectTextureCount(); i++) {
        M_FillObjectUVW(i);
    }
}

static void M_FillSpriteUVWs(void)
{
    for (int32_t i = 0; i < Output_GetSpriteTextureCount(); i++) {
        M_FillSpriteUVW(i);
    }
}

static void M_UpdateObjectAnimatedUVWs(VECTOR *const source)
{
    for (int32_t i = 0; i < source->count; i++) {
        const OUTPUT_VERTEX_RANGE *const range = Vector_Get(source, i);
        for (int32_t j = 0; j < range->vertex_count; j++) {
            M_FillObjectUVW(range->vertex_start + j);
            M_FillAtlasObjectSize(range->vertex_start + j);
        }
    }
}

static void M_UpdateSpriteAnimatedUVWs(VECTOR *const source)
{
    for (int32_t i = 0; i < source->count; i++) {
        const OUTPUT_VERTEX_RANGE *const range = Vector_Get(source, i);
        for (int32_t j = 0; j < range->vertex_count; j++) {
            M_FillSpriteUVW(range->vertex_start + j);
            M_FillAtlasSpriteSize(range->vertex_start + j);
        }
    }
}

static void M_PrepareUVWs(void)
{
    m_Priv.uvws.count_objects = Output_GetObjectTextureCount();
    m_Priv.uvws.count_sprites = Output_GetSpriteTextureCount();
    m_Priv.uvws.count = m_Priv.uvws.count_objects + m_Priv.uvws.count_sprites;
    m_Priv.uvws.data = Memory_Alloc(m_Priv.uvws.count * sizeof(M_UVW_PACK));
    m_Priv.uvws.data_objects = m_Priv.uvws.data;
    m_Priv.uvws.data_sprites = m_Priv.uvws.data + m_Priv.uvws.count_objects;
    m_Priv.uvws.animated_objects =
        Memory_Alloc(m_Priv.uvws.count_objects * sizeof(bool));
    m_Priv.uvws.animated_sprites =
        Memory_Alloc(m_Priv.uvws.count_sprites * sizeof(bool));
    M_FillObjectUVWs();
    M_FillSpriteUVWs();
}

static void M_PrepareEnvMap(void)
{
    glGenTextures(1, &m_Priv.tex_env_map);
    glBindTexture(GL_TEXTURE_2D, m_Priv.tex_env_map);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GFX_GL_CheckError();
}

static void M_PrepareAtlasSizes(void)
{
    const int32_t count_objects = Output_GetObjectTextureCount();
    const int32_t count_sprites = Output_GetSpriteTextureCount();
    const int32_t count = count_objects + count_sprites;
    m_Priv.atlas_sizes.data = Memory_Realloc(
        m_Priv.atlas_sizes.data, count * sizeof(OUTPUT_TEXTURE_SIZE));
    m_Priv.atlas_sizes.data_objects = m_Priv.atlas_sizes.data;
    m_Priv.atlas_sizes.data_sprites = m_Priv.atlas_sizes.data + count_objects;
    for (int32_t i = 0; i < count_objects; i++) {
        M_FillAtlasObjectSize(i);
    }
    for (int32_t i = 0; i < count_sprites; i++) {
        M_FillAtlasSpriteSize(i);
    }
}

static void M_UploadAtlas(void)
{
    glGenTextures(1, &m_Priv.tex_atlas);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_Priv.tex_atlas);
    glTexStorage3D(
        GL_TEXTURE_2D_ARRAY,
        1, // number of mipmaps
        GL_RGBA8, TEXTURE_PAGE_WIDTH, TEXTURE_PAGE_HEIGHT,
        Output_GetTexturePageCount());
    GFX_GL_CheckError();

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GFX_GL_CheckError();

    for (int32_t i = 0; i < Output_GetTexturePageCount(); i++) {
        const RGBA_8888 *const input_ptr = Output_GetTexturePage32(i);

        glTexSubImage3D(
            GL_TEXTURE_2D_ARRAY,
            0, // mipmap level
            0, // x offset
            0, // y offset
            i, // z offset
            TEXTURE_PAGE_WIDTH, TEXTURE_PAGE_HEIGHT,
            1, // depth
            GL_RGBA, GL_UNSIGNED_BYTE, input_ptr);
    }
    GFX_GL_CheckError();

    M_PrepareAtlasSizes();

    GFX_GL_CheckError();
}

static void M_FreeLevelData(void)
{
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    if (m_Priv.tex_atlas != 0) {
        glDeleteTextures(1, &m_Priv.tex_atlas);
        m_Priv.tex_atlas = 0;
    }
    Memory_FreePointer(&m_Priv.uvws.data);
    Memory_FreePointer(&m_Priv.uvws.animated_objects);
    Memory_FreePointer(&m_Priv.uvws.animated_sprites);
    Memory_FreePointer(&m_Priv.atlas_sizes.data);
}

void Output_Textures_Init(void)
{
    M_PrepareEnvMap();
    m_AnimationRanges.objects = Vector_Create(sizeof(OUTPUT_VERTEX_RANGE));
    m_AnimationRanges.sprites = Vector_Create(sizeof(OUTPUT_VERTEX_RANGE));
}

void Output_Textures_Shutdown(void)
{
    Vector_Free(m_AnimationRanges.objects);
    Vector_Free(m_AnimationRanges.sprites);
    M_FreeLevelData();
    if (m_Priv.tex_env_map != 0) {
        glDeleteTextures(1, &m_Priv.tex_env_map);
        m_Priv.tex_env_map = 0;
    }
}

void Output_Textures_ObserveLevelLoad(void)
{
    M_FreeLevelData();
    M_PrepareUVWs();
    M_PrepareAnimationRanges();
    M_UploadAtlas();
}

void Output_Textures_UpdateEnvironmentMap(void)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    GFX_GL_CheckError();

    const GLint vp_x = viewport[0];
    const GLint vp_y = viewport[1];
    const GLint vp_w = viewport[2];
    const GLint vp_h = viewport[3];

    const int32_t side = MIN(vp_w, vp_h);
    const int32_t x = vp_x + (vp_w - side) / 2;
    const int32_t y = vp_y + (vp_h - side) / 2;
    const int32_t w = side;
    const int32_t h = side;

    glBindTexture(GL_TEXTURE_2D, m_Priv.tex_env_map);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, w, h, 0);
    GFX_GL_CheckError();
}

void Output_Textures_CycleAnimations(void)
{
    if (m_Priv.uvws.count != 0) {
        M_UpdateSpriteAnimatedUVWs(m_AnimationRanges.sprites);
        M_UpdateObjectAnimatedUVWs(m_AnimationRanges.objects);
    }
}

GLuint Output_Textures_GetAtlasTexture(void)
{
    return m_Priv.tex_atlas;
}

GLuint Output_Textures_GetEnvMapTexture(void)
{
    return m_Priv.tex_env_map;
}

int32_t Output_Textures_GetSpritesUVWsBase(void)
{
    const size_t num = sizeof(M_UVW_PACK) / sizeof(OUTPUT_UVW);
    ASSERT(num == 4);
    return m_Priv.uvws.count_objects * num;
}

OUTPUT_UVW Output_Textures_GetUVW(const int32_t uvw_idx)
{
    return m_Priv.uvws.data[uvw_idx / 4].corners[uvw_idx % 4];
}

OUTPUT_TEXTURE_SIZE Output_Textures_GetAtlasSize(const int32_t uvw_idx)
{
    return m_Priv.atlas_sizes.data[uvw_idx];
}

bool Output_Textures_IsObjectTextureAnimated(const int32_t texture_idx)
{
    return m_Priv.uvws.animated_objects[texture_idx];
}

bool Output_Textures_IsSpriteTextureAnimated(const int32_t uvw_idx)
{
    return m_Priv.uvws.animated_sprites[uvw_idx];
}

void Output_Textures_ApplyRenderSettings(void)
{
    // re-adjust UVs when the bilinear filter is toggled.
    if (m_Priv.uvws.count != 0) {
        M_FillObjectUVWs();
    }
}
