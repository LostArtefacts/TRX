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

static struct {
    M_ANIMATION_RANGES objects;
    M_ANIMATION_RANGES sprites;
} m_AnimationRanges;

static struct {
    GLuint tex_atlas;
    GLuint tex_env_map;

    struct {
        GLuint tbo; // buffer to hold UV data
        GLuint tex; // texture to hold UV buffer
        int32_t count;
        int32_t count_objects;
        int32_t count_sprites;
        M_UVW_PACK *data;
        M_UVW_PACK *data_objects;
        M_UVW_PACK *data_sprites;
    } uvws;

    struct {
        GLuint tex;
        GLuint tbo;
        M_ATLAS_SIZE *data;
        M_ATLAS_SIZE *data_objects;
        M_ATLAS_SIZE *data_sprites;
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

static void M_FillAtlasObjectSize(const int32_t i)
{
    M_ATLAS_SIZE *const size = &m_Priv.atlas_sizes.data_objects[i];
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
    M_ATLAS_SIZE *const size = &m_Priv.atlas_sizes.data_sprites[i];
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(i);
    const float adj = 0.1 / 256.0f;
    const float u0 = (sprite->offset & 0xFF) / 256.0f + adj;
    const float v0 = (sprite->offset >> 8) / 256.0f + adj;
    const float u1 = u0 + (sprite->width >> 8) / 256.0f - 2 * adj;
    const float v1 = v0 + (sprite->height >> 8) / 256.0f - 2 * adj;
    size->x0 = u0;
    size->y0 = v0;
    size->x1 = u1;
    size->y1 = v1;
}

static void M_FillObjectUVW(const int32_t i)
{
    const OBJECT_TEXTURE *const texture = Output_GetObjectTexture(i);
    M_UVW *const corners = m_Priv.uvws.data_objects[i].corners;
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
    M_UVW *const corners = m_Priv.uvws.data_sprites[i].corners;
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

static void M_UploadUVWs(void)
{
    glBindBuffer(GL_TEXTURE_BUFFER, m_Priv.uvws.tbo);
    GFX_TRACK_DATA(
        glBufferData, GL_TEXTURE_BUFFER, m_Priv.uvws.count * sizeof(M_UVW_PACK),
        m_Priv.uvws.data, GL_DYNAMIC_DRAW);
    GFX_GL_CheckError();
}

static void M_UploadObjectAnimatedUVWs(const M_ANIMATION_RANGES *const source)
{
    glBindBuffer(GL_TEXTURE_BUFFER, m_Priv.uvws.tbo);
    for (int32_t i = 0; i < source->range_count; i++) {
        const M_ANIMATION_RANGE *const range = &source->ranges[i];
        for (int32_t j = 0; j < range->count; j++) {
            M_FillObjectUVW(range->index + j);
        }
        const M_UVW_PACK *const source =
            m_Priv.uvws.data_objects + range->index;
        GFX_TRACK_DATA(
            glBufferSubData, GL_TEXTURE_BUFFER,
            (source - m_Priv.uvws.data) * sizeof(M_UVW_PACK),
            range->count * sizeof(M_UVW_PACK), source);
    }

    glBindBuffer(GL_TEXTURE_BUFFER, m_Priv.atlas_sizes.tbo);
    for (int32_t i = 0; i < source->range_count; i++) {
        const M_ANIMATION_RANGE *const range = &source->ranges[i];
        for (int32_t j = 0; j < range->count; j++) {
            M_FillAtlasObjectSize(range->index + j);
        }
        const M_ATLAS_SIZE *const source =
            m_Priv.atlas_sizes.data_objects + range->index;
        GFX_TRACK_DATA(
            glBufferSubData, GL_TEXTURE_BUFFER,
            (source - m_Priv.atlas_sizes.data) * sizeof(M_ATLAS_SIZE),
            range->count * sizeof(M_ATLAS_SIZE), source);
    }
}

static void M_UploadSpriteAnimatedUVWs(const M_ANIMATION_RANGES *const source)
{
    glBindBuffer(GL_TEXTURE_BUFFER, m_Priv.uvws.tbo);
    for (int32_t i = 0; i < source->range_count; i++) {
        const M_ANIMATION_RANGE *const range = &source->ranges[i];
        for (int32_t j = 0; j < range->count; j++) {
            M_FillSpriteUVW(range->index + j);
        }
        const M_UVW_PACK *const source =
            m_Priv.uvws.data_sprites + range->index;
        GFX_TRACK_DATA(
            glBufferSubData, GL_TEXTURE_BUFFER,
            (source - m_Priv.uvws.data) * sizeof(M_UVW_PACK),
            range->count * sizeof(M_UVW_PACK), source);
    }

    glBindBuffer(GL_TEXTURE_BUFFER, m_Priv.atlas_sizes.tbo);
    for (int32_t i = 0; i < source->range_count; i++) {
        const M_ANIMATION_RANGE *const range = &source->ranges[i];
        for (int32_t j = 0; j < range->count; j++) {
            M_FillAtlasSpriteSize(range->index + j);
        }
        const M_ATLAS_SIZE *const source =
            m_Priv.atlas_sizes.data_sprites + range->index;
        GFX_TRACK_DATA(
            glBufferSubData, GL_TEXTURE_BUFFER,
            (source - m_Priv.atlas_sizes.data) * sizeof(M_ATLAS_SIZE),
            range->count * sizeof(M_ATLAS_SIZE), source);
    }
}

static void M_PrepareUVWBuffers(void)
{
    m_Priv.uvws.count_objects = Output_GetObjectTextureCount();
    m_Priv.uvws.count_sprites = Output_GetSpriteTextureCount();
    m_Priv.uvws.count = m_Priv.uvws.count_objects + m_Priv.uvws.count_sprites;
    m_Priv.uvws.data = Memory_Alloc(m_Priv.uvws.count * sizeof(M_UVW_PACK));
    m_Priv.uvws.data_objects = m_Priv.uvws.data;
    m_Priv.uvws.data_sprites = m_Priv.uvws.data + m_Priv.uvws.count_objects;

    glGenBuffers(1, &m_Priv.uvws.tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, m_Priv.uvws.tbo);

    GLint limit;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &limit);
    ASSERT(m_Priv.uvws.count * sizeof(M_UVW_PACK) <= (size_t)limit);

    glGenTextures(1, &m_Priv.uvws.tex);
    glBindTexture(GL_TEXTURE_BUFFER, m_Priv.uvws.tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_Priv.uvws.tbo);
    GFX_GL_CheckError();
}

static void M_PrepareUVWs(void)
{
    M_PrepareUVWBuffers();
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
    glGenBuffers(1, &m_Priv.atlas_sizes.tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, m_Priv.atlas_sizes.tbo);
    glGenTextures(1, &m_Priv.atlas_sizes.tex);
    glBindTexture(GL_TEXTURE_BUFFER, m_Priv.atlas_sizes.tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_Priv.atlas_sizes.tbo);
    const int32_t count_objects = Output_GetObjectTextureCount();
    const int32_t count_sprites = Output_GetSpriteTextureCount();
    const int32_t count = count_objects + count_sprites;
    m_Priv.atlas_sizes.data =
        Memory_Realloc(m_Priv.atlas_sizes.data, count * sizeof(M_ATLAS_SIZE));
    m_Priv.atlas_sizes.data_objects = m_Priv.atlas_sizes.data;
    m_Priv.atlas_sizes.data_sprites = m_Priv.atlas_sizes.data + count_objects;
    for (int32_t i = 0; i < count_objects; i++) {
        M_FillAtlasObjectSize(i);
    }
    for (int32_t i = 0; i < count_sprites; i++) {
        M_FillAtlasSpriteSize(i);
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
    if (m_Priv.uvws.tbo != 0) {
        glDeleteBuffers(1, &m_Priv.uvws.tbo);
        m_Priv.uvws.tbo = 0;
    }
    if (m_Priv.uvws.tex != 0) {
        glDeleteTextures(1, &m_Priv.uvws.tex);
        m_Priv.uvws.tex = 0;
    }
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
    M_PrepareUVWs();
    M_UploadUVWs();
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
    if (m_Priv.uvws.tex != 0) {
        M_UploadSpriteAnimatedUVWs(&m_AnimationRanges.sprites);
        M_UploadObjectAnimatedUVWs(&m_AnimationRanges.objects);
    }
}

GLuint Output_Textures_GetUVWsTexture(void)
{
    return m_Priv.uvws.tex;
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

int32_t Output_Textures_GetSpritesUVWsBase(void)
{
    const size_t num = sizeof(M_UVW_PACK) / sizeof(M_UVW);
    ASSERT(num == 4);
    return m_Priv.uvws.count_objects * num;
}

void Output_Textures_ApplyRenderSettings(void)
{
    // re-adjust UVs when the bilinear filter is toggled.
    if (m_Priv.uvws.tex != 0) {
        M_FillObjectUVWs();
        M_UploadUVWs();
    }
}
