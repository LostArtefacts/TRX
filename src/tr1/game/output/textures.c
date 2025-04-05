#include "game/output/textures.h"

#include "game/output.h"

#include <libtrx/debug.h>
#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>

#include <stdlib.h>

#pragma pack(push, 1)
typedef struct {
    float x0, y0, x1, y1;
} M_ATLAS_SIZE;
#pragma pack(pop)

typedef struct {
    int32_t index;
    int32_t count;
} M_ANIMATION_RANGE;

typedef struct {
    int32_t range_count;
    M_ANIMATION_RANGE *ranges;
} M_ANIMATION_RANGES;

typedef struct {
    float u;
    float v;
    float w;
} M_UVW;

typedef struct {
    M_UVW corners[4];
} M_UVW_PACK;

typedef struct {
    GLuint tbo; // buffer to hold UV data
    GLuint tex; // texture to hold UV buffer
    int32_t count;
    M_UVW_PACK *uvw;
} M_TEXTURE_DATA;

static struct {
    M_ANIMATION_RANGES objects;
    M_ANIMATION_RANGES sprites;
} m_AnimationRanges;

static struct {
    GLuint tex_atlas;
    GLuint tex_env_map;
    M_TEXTURE_DATA objects;
    M_TEXTURE_DATA sprites;
    struct {
        GLuint tex;
        GLuint tbo;
        M_ATLAS_SIZE *data;
    } atlas_sizes;
} m_Priv = {};

static int M_CompareAnimationRange(const void *const a, const void *const b)
{
    const M_ANIMATION_RANGE *const range_a = (M_ANIMATION_RANGE *)a;
    const M_ANIMATION_RANGE *const range_b = (M_ANIMATION_RANGE *)b;
    return range_a->index - range_b->index;
}

static void M_MergeAndGlueAnimationRanges(M_ANIMATION_RANGES *const source)
{
    ASSERT(source != nullptr);
    if (source->range_count == 0) {
        return;
    }

    // Sort ranges by index
    qsort(
        source->ranges, source->range_count, sizeof(M_ANIMATION_RANGE),
        M_CompareAnimationRange);

    // Initialize a new index to store the merged ranges
    int32_t new_range_count = 0;

    // Iterate over sorted ranges and merge them
    for (int32_t i = 0; i < source->range_count; i++) {
        if (new_range_count == 0) {
            // First range - just copy it
            source->ranges[new_range_count++] = source->ranges[i];
        } else {
            // Check if the previous range can be merged with the current one
            M_ANIMATION_RANGE *const last_range =
                &source->ranges[new_range_count - 1];
            const int32_t last_start = last_range->index;
            const int32_t last_end = last_range->index + last_range->count;
            const int32_t current_start = source->ranges[i].index;
            const int32_t current_end =
                source->ranges[i].index + source->ranges[i].count;

            if (current_start >= last_start && current_start <= last_end) {
                last_range->count = current_end - last_range->index;
            } else if (current_end >= last_start && current_end <= last_end) {
                last_range->index = source->ranges[i].index;
            } else {
                source->ranges[new_range_count++] = source->ranges[i];
            }
        }
    }

    // Update the range count with the new number of merged ranges
    source->range_count = new_range_count;
}

static void M_PrepareObjectAnimationRanges(void)
{
    m_AnimationRanges.objects.range_count = 0;
    for (const ANIMATED_TEXTURE_RANGE *src_range =
             Output_GetAnimatedTextureRange(0);
         src_range != nullptr; src_range = src_range->next_range) {
        m_AnimationRanges.objects.range_count += src_range->num_textures;
    }

    m_AnimationRanges.objects.ranges = Memory_Realloc(
        m_AnimationRanges.objects.ranges,
        m_AnimationRanges.objects.range_count * sizeof(M_ANIMATION_RANGE));

    M_ANIMATION_RANGE *dst_range = m_AnimationRanges.objects.ranges;
    for (const ANIMATED_TEXTURE_RANGE *src_range =
             Output_GetAnimatedTextureRange(0);
         src_range != nullptr; src_range = src_range->next_range) {
        for (int32_t i = 0; i < src_range->num_textures; i++) {
            dst_range->index = src_range->textures[i];
            dst_range->count = 1;
            dst_range++;
        }
    }
    M_MergeAndGlueAnimationRanges(&m_AnimationRanges.objects);
}

static void M_PrepareSpriteAnimationRanges(void)
{
    m_AnimationRanges.sprites.range_count = 0;
    for (int32_t i = 0; i < MAX_STATIC_OBJECTS; i++) {
        const STATIC_OBJECT_2D *const obj = Object_Get2DStatic(i);
        if (!obj->loaded || obj->frame_count == 1) {
            continue;
        }
        m_AnimationRanges.sprites.range_count++;
    }

    m_AnimationRanges.sprites.ranges = Memory_Realloc(
        m_AnimationRanges.sprites.ranges,
        m_AnimationRanges.sprites.range_count * sizeof(M_ANIMATION_RANGE));

    M_ANIMATION_RANGE *dst_range = m_AnimationRanges.sprites.ranges;
    for (int32_t i = 0; i < MAX_STATIC_OBJECTS; i++) {
        const STATIC_OBJECT_2D *const obj = Object_Get2DStatic(i);
        if (!obj->loaded || obj->frame_count == 1) {
            continue;
        }
        dst_range->index = obj->texture_idx;
        dst_range->count = obj->frame_count;
        dst_range++;
    }
    M_MergeAndGlueAnimationRanges(&m_AnimationRanges.sprites);
}

static void M_PrepareAnimationRanges(void)
{
    M_PrepareObjectAnimationRanges();
    M_PrepareSpriteAnimationRanges();
}

static void M_FreeTextureData(M_TEXTURE_DATA *const data)
{
    if (data->tbo != 0) {
        glDeleteBuffers(1, &data->tbo);
        data->tbo = 0;
    }
    if (data->tex != 0) {
        glDeleteTextures(1, &data->tex);
        data->tex = 0;
    }
}

static void M_FillAtlasSize(const int32_t i)
{
    M_ATLAS_SIZE *size = &m_Priv.atlas_sizes.data[i];
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

static void M_FillObjectUVW(const int32_t i)
{
    const OBJECT_TEXTURE *const texture = Output_GetObjectTexture(i);
    M_UVW *const corners = m_Priv.objects.uvw[i].corners;
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
    const float u1 = u0 + (sprite->width >> 8) / 256.0f - 2 * adj;
    const float v1 = v0 + (sprite->height >> 8) / 256.0f - 2 * adj;
    M_UVW *const corners = m_Priv.sprites.uvw[i].corners;
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

static void M_UploadTextureData(const M_TEXTURE_DATA *const data)
{
    glBindBuffer(GL_TEXTURE_BUFFER, data->tbo);
    GFX_TRACK_DATA(
        glBufferData, GL_TEXTURE_BUFFER, data->count * sizeof(M_UVW_PACK),
        data->uvw, GL_DYNAMIC_DRAW);
    GFX_GL_CheckError();
}

static void M_UploadObjectUVWs(void)
{
    M_UploadTextureData(&m_Priv.objects);
}

static void M_UploadSpriteUVWs(void)
{
    M_UploadTextureData(&m_Priv.sprites);
}

static void M_UploadObjectAnimatedUVWs(const M_ANIMATION_RANGES *const source)
{
    glBindBuffer(GL_TEXTURE_BUFFER, m_Priv.objects.tbo);
    for (int32_t i = 0; i < source->range_count; i++) {
        const M_ANIMATION_RANGE *const range = &source->ranges[i];
        for (int32_t j = 0; j < range->count; j++) {
            M_FillObjectUVW(range->index + j);
        }
        GFX_TRACK_DATA(
            glBufferSubData, GL_TEXTURE_BUFFER,
            range->index * sizeof(M_UVW_PACK),
            range->count * sizeof(M_UVW_PACK),
            m_Priv.objects.uvw + range->index);
    }

    glBindBuffer(GL_TEXTURE_BUFFER, m_Priv.atlas_sizes.tbo);
    for (int32_t i = 0; i < source->range_count; i++) {
        const M_ANIMATION_RANGE *const range = &source->ranges[i];
        for (int32_t j = 0; j < range->count; j++) {
            M_FillAtlasSize(range->index + j);
        }
        GFX_TRACK_DATA(
            glBufferSubData, GL_TEXTURE_BUFFER,
            range->index * sizeof(M_ATLAS_SIZE),
            range->count * sizeof(M_ATLAS_SIZE),
            m_Priv.atlas_sizes.data + range->index);
    }
}

static void M_UploadSpriteAnimatedUVWs(const M_ANIMATION_RANGES *const source)
{
    glBindBuffer(GL_TEXTURE_BUFFER, m_Priv.sprites.tbo);
    for (int32_t i = 0; i < source->range_count; i++) {
        const M_ANIMATION_RANGE *const range = &source->ranges[i];
        for (int32_t j = 0; j < range->count; j++) {
            M_FillSpriteUVW(range->index + j);
        }
        GFX_TRACK_DATA(
            glBufferSubData, GL_TEXTURE_BUFFER,
            range->index * sizeof(M_UVW_PACK),
            range->count * sizeof(M_UVW_PACK),
            m_Priv.sprites.uvw + range->index);
    }
}

static void M_PrepareTextureData(M_TEXTURE_DATA *const data, const size_t count)
{
    data->count = count;
    data->uvw = Memory_Alloc(data->count * sizeof(M_UVW_PACK));

    glGenBuffers(1, &data->tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, data->tbo);

    GLint limit;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &limit);
    ASSERT(data->count * sizeof(M_UVW_PACK) <= (size_t)limit);

    glGenTextures(1, &data->tex);
    glBindTexture(GL_TEXTURE_BUFFER, data->tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, data->tbo);
    GFX_GL_CheckError();
}

static void M_PrepareObjectUVWs(void)
{
    M_PrepareTextureData(&m_Priv.objects, Output_GetObjectTextureCount());
    M_FillObjectUVWs();
}

static void M_PrepareSpriteUVWs(void)
{
    M_PrepareTextureData(&m_Priv.sprites, Output_GetSpriteTextureCount());
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
    glGenBuffers(1, &m_Priv.atlas_sizes.tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, m_Priv.atlas_sizes.tbo);
    glGenTextures(1, &m_Priv.atlas_sizes.tex);
    glBindTexture(GL_TEXTURE_BUFFER, m_Priv.atlas_sizes.tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_Priv.atlas_sizes.tbo);
    const int32_t count = Output_GetObjectTextureCount();
    m_Priv.atlas_sizes.data =
        Memory_Realloc(m_Priv.atlas_sizes.data, count * sizeof(M_ATLAS_SIZE));
    for (int32_t i = 0; i < count; i++) {
        M_FillAtlasSize(i);
    }
    GFX_TRACK_DATA(
        glBufferData, GL_TEXTURE_BUFFER, count * sizeof(M_ATLAS_SIZE),
        m_Priv.atlas_sizes.data, GL_DYNAMIC_DRAW);
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
    M_FreeTextureData(&m_Priv.objects);
    M_FreeTextureData(&m_Priv.sprites);
    if (m_Priv.tex_atlas != 0) {
        glDeleteTextures(1, &m_Priv.tex_atlas);
        m_Priv.tex_atlas = 0;
    }
    if (m_Priv.atlas_sizes.tex != 0) {
        glDeleteTextures(1, &m_Priv.atlas_sizes.tex);
        m_Priv.atlas_sizes.tex = 0;
    }
    if (m_Priv.atlas_sizes.tbo != 0) {
        glDeleteBuffers(1, &m_Priv.atlas_sizes.tbo);
        m_Priv.atlas_sizes.tbo = 0;
    }
    Memory_FreePointer(&m_Priv.atlas_sizes.data);
    Memory_FreePointer(&m_AnimationRanges.objects.ranges);
    Memory_FreePointer(&m_AnimationRanges.sprites.ranges);
}

void Output_Textures_Init(void)
{
    M_PrepareEnvMap();
}

void Output_Textures_Shutdown(void)
{
    M_FreeLevelData();
    if (m_Priv.tex_env_map != 0) {
        glDeleteTextures(1, &m_Priv.tex_env_map);
        m_Priv.tex_env_map = 0;
    }
}

void Output_Textures_ObserveLevelLoad(void)
{
    M_FreeLevelData();
    M_PrepareAnimationRanges();
    M_PrepareObjectUVWs();
    M_PrepareSpriteUVWs();
    M_UploadObjectUVWs();
    M_UploadSpriteUVWs();
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
    if (m_Priv.sprites.tex != 0) {
        M_UploadSpriteAnimatedUVWs(&m_AnimationRanges.sprites);
    }
    if (m_Priv.objects.tex != 0) {
        M_UploadObjectAnimatedUVWs(&m_AnimationRanges.objects);
    }
}

GLuint Output_Textures_GetObjectUVWsTexture(void)
{
    return m_Priv.objects.tex;
}

GLuint Output_Textures_GetSpriteUVWsTexture(void)
{
    return m_Priv.sprites.tex;
}

GLuint Output_Textures_GetAtlasTexture(void)
{
    return m_Priv.tex_atlas;
}

GLuint Output_Textures_GetAtlasSizesTexture(void)
{
    return m_Priv.atlas_sizes.tex;
}

GLuint Output_Textures_GetEnvMapTexture(void)
{
    return m_Priv.tex_env_map;
}

void Output_Textures_ApplyRenderSettings(void)
{
    // re-adjust UVs when the bilinear filter is toggled.
    if (m_Priv.objects.tex != 0) {
        M_FillObjectUVWs();
        M_UploadObjectUVWs();
    }
}
