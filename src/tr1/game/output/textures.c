#include "game/output/textures.h"

#include "game/output.h"

#include <libtrx/debug.h>
#include <libtrx/gfx/gl/utils.h>
#include <libtrx/memory.h>

#include <stdlib.h>

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
    GLuint tex; // 3D texture to hold atlas pages
    M_TEXTURE_DATA sprites;
} m_LevelData = {};

int M_CompareAnimationRange(const void *const a, const void *const b)
{
    const M_ANIMATION_RANGE *const range_a = (M_ANIMATION_RANGE *)a;
    const M_ANIMATION_RANGE *const range_b = (M_ANIMATION_RANGE *)b;
    return range_a->index - range_b->index;
}

void M_MergeAndGlueAnimationRanges(M_ANIMATION_RANGES *const source)
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

void Output_Textures_Init(void)
{
}

void Output_Textures_Shutdown(void)
{
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    if (m_LevelData.sprites.tbo != 0) {
        glDeleteBuffers(1, &m_LevelData.sprites.tbo);
        m_LevelData.sprites.tbo = 0;
    }
    if (m_LevelData.sprites.tex != 0) {
        glDeleteTextures(1, &m_LevelData.sprites.tex);
        m_LevelData.sprites.tex = 0;
    }
    if (m_LevelData.tex != 0) {
        glDeleteTextures(1, &m_LevelData.tex);
        m_LevelData.tex = 0;
    }

    Memory_FreePointer(&m_AnimationRanges.objects.ranges);
    Memory_FreePointer(&m_AnimationRanges.sprites.ranges);
}

static void M_FillSpriteUVW(const int32_t i)
{
    const SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(i);
    const float adj = 0.1 / 256.0f;
    const float u0 = (sprite->offset & 0xFF) / 256.0f + adj;
    const float v0 = (sprite->offset >> 8) / 256.0f + adj;
    const float u1 = u0 + (sprite->width >> 8) / 256.0f - 2 * adj;
    const float v1 = v0 + (sprite->height >> 8) / 256.0f - 2 * adj;
    M_UVW *corners = m_LevelData.sprites.uvw[i].corners;
    // clang-format off
    corners[0].u = u0; corners[0].v = v0; corners[0].w = sprite->tex_page;
    corners[1].u = u1; corners[1].v = v0; corners[1].w = sprite->tex_page;
    corners[2].u = u1; corners[2].v = v1; corners[2].w = sprite->tex_page;
    corners[3].u = u0; corners[3].v = v1; corners[3].w = sprite->tex_page;
    // clang-format on
}

static void M_FillSpriteUVWs(void)
{
    for (int32_t i = 0; i < Output_GetSpriteTextureCount(); i++) {
        M_FillSpriteUVW(i);
    }
}

static void M_UploadSpriteUVWs(void)
{
    glBindBuffer(GL_TEXTURE_BUFFER, m_LevelData.sprites.tbo);
    GFX_TRACK_DATA(
        glBufferData, GL_TEXTURE_BUFFER,
        m_LevelData.sprites.count * sizeof(M_UVW_PACK), m_LevelData.sprites.uvw,
        GL_DYNAMIC_DRAW);
}

static void M_UploadSpriteAnimatedUVWs(const M_ANIMATION_RANGES *const source)
{
    glBindBuffer(GL_TEXTURE_BUFFER, m_LevelData.sprites.tbo);
    for (int32_t i = 0; i < source->range_count; i++) {
        const M_ANIMATION_RANGE *const range = &source->ranges[i];
        for (int32_t j = 0; j < range->count; j++) {
            M_FillSpriteUVW(range->index + j);
        }
        GFX_TRACK_DATA(
            glBufferSubData, GL_TEXTURE_BUFFER,
            range->index * sizeof(M_UVW_PACK),
            range->count * sizeof(M_UVW_PACK),
            m_LevelData.sprites.uvw + range->index);
    }
}

static void M_PrepareSpriteUVWs(void)
{
    m_LevelData.sprites.count = Output_GetSpriteTextureCount();
    m_LevelData.sprites.uvw =
        Memory_Alloc(m_LevelData.sprites.count * sizeof(M_UVW_PACK));
    M_FillSpriteUVWs();

    glGenBuffers(1, &m_LevelData.sprites.tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, m_LevelData.sprites.tbo);

    GLint limit;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &limit);
    ASSERT(m_LevelData.sprites.count * sizeof(M_UVW_PACK) <= (size_t)limit);

    glGenTextures(1, &m_LevelData.sprites.tex);
    glBindTexture(GL_TEXTURE_BUFFER, m_LevelData.sprites.tex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, m_LevelData.sprites.tbo);
}

static void M_UploadAtlas(void)
{
    glGenTextures(1, &m_LevelData.tex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_LevelData.tex);
    glTexStorage3D(
        GL_TEXTURE_2D_ARRAY,
        1, // number of mipmaps
        GL_RGBA8, TEXTURE_PAGE_WIDTH, TEXTURE_PAGE_HEIGHT,
        Output_GetTexturePageCount());

    // TODO: handle bilinear toggle
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
}

void Output_Textures_ObserveLevelLoad(void)
{
    Output_Textures_Shutdown();
    M_PrepareAnimationRanges();
    M_PrepareSpriteUVWs();
    M_UploadSpriteUVWs();
    M_UploadAtlas();
}

void Output_Textures_Update(void)
{
    if (m_LevelData.sprites.tex == 0) {
        return;
    }
    M_UploadSpriteAnimatedUVWs(&m_AnimationRanges.sprites);
}

GLuint Output_Textures_GetSpriteUVWsTexture(void)
{
    return m_LevelData.sprites.tex;
}

GLuint Output_Textures_GetAtlasTexture(void)
{
    return m_LevelData.tex;
}
