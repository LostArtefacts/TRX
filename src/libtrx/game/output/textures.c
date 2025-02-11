#include "debug.h"
#include "game/const.h"
#include "game/game_buf.h"
#include "game/objects/common.h"
#include "game/output.h"
#include "game/shell.h"
#include "utils.h"

#include <string.h>

static int32_t m_TexturePageCount = 0;
static uint8_t *m_TexturePages8 = nullptr;
static RGBA_8888 *m_TexturePages32 = nullptr;

static int32_t m_PaletteSize = 0;
static RGB_888 *m_Palette8 = nullptr;
static RGB_888 *m_Palette16 = nullptr;

static LIGHT_MAP m_LightMap[32];
static SHADE_MAP m_ShadeMap[256];

static int32_t m_ObjectTextureCount = 0;
static OBJECT_TEXTURE *m_ObjectTextures = nullptr;
static SPRITE_TEXTURE *m_SpriteTextures = nullptr;
static ANIMATED_TEXTURE_RANGE *m_AnimTextureRanges = nullptr;

void Output_InitialiseTexturePages(const int32_t num_pages, const bool use_8bit)
{
    m_TexturePageCount = num_pages;
    if (num_pages == 0) {
        m_TexturePages32 = nullptr;
        m_TexturePages8 = nullptr;
        return;
    }

    const int32_t page_size = num_pages * TEXTURE_PAGE_SIZE;
    m_TexturePages32 =
        GameBuf_Alloc(sizeof(RGBA_8888) * page_size, GBUF_TEXTURE_PAGES);
    m_TexturePages8 = use_8bit
        ? GameBuf_Alloc(sizeof(uint8_t) * page_size, GBUF_TEXTURE_PAGES)
        : nullptr;
}

void Output_InitialisePalettes(
    const int32_t palette_size, const RGB_888 *const palette_8,
    const RGB_888 *const palette_16)
{
    ASSERT(palette_size != 0);
    ASSERT(palette_8 != nullptr);
    m_PaletteSize = palette_size;

    m_Palette8 = GameBuf_Alloc(sizeof(RGB_888) * palette_size, GBUF_PALETTES);
    memcpy(m_Palette8, palette_8, sizeof(RGB_888) * palette_size);

    if (palette_16 != nullptr) {
        m_Palette16 =
            GameBuf_Alloc(sizeof(RGB_888) * palette_size, GBUF_PALETTES);
        memcpy(m_Palette16, palette_16, sizeof(RGB_888) * palette_size);
    } else {
        m_Palette16 = nullptr;
    }
}

void Output_InitialiseObjectTextures(const int32_t num_textures)
{
    m_ObjectTextureCount = num_textures;
    m_ObjectTextures = num_textures == 0
        ? nullptr
        : GameBuf_Alloc(
              sizeof(OBJECT_TEXTURE) * num_textures, GBUF_OBJECT_TEXTURES);
}

void Output_InitialiseSpriteTextures(const int32_t num_textures)
{
    m_SpriteTextures = num_textures == 0
        ? nullptr
        : GameBuf_Alloc(
              sizeof(SPRITE_TEXTURE) * num_textures, GBUF_SPRITE_TEXTURES);
}

void Output_InitialiseAnimatedTextures(const int32_t num_ranges)
{
    m_AnimTextureRanges = num_ranges == 0
        ? nullptr
        : GameBuf_Alloc(
              sizeof(ANIMATED_TEXTURE_RANGE) * num_ranges,
              GBUF_ANIMATED_TEXTURE_RANGES);
}

int32_t Output_GetTexturePageCount(void)
{
    return m_TexturePageCount;
}

uint8_t *Output_GetTexturePage8(const int32_t page_idx)
{
    if (m_TexturePages8 == nullptr) {
        return nullptr;
    }
    return &m_TexturePages8[page_idx * TEXTURE_PAGE_SIZE];
}

RGBA_8888 *Output_GetTexturePage32(const int32_t page_idx)
{
    if (m_TexturePages32 == nullptr) {
        return nullptr;
    }
    return &m_TexturePages32[page_idx * TEXTURE_PAGE_SIZE];
}

int32_t Output_GetPaletteSize(void)
{
    return m_PaletteSize;
}

RGB_888 Output_GetPaletteColor8(const uint16_t idx)
{
    if (m_Palette8 == nullptr) {
        return (RGB_888) { 0, 0, 0 };
    }
    return m_Palette8[idx];
}

RGB_888 Output_GetPaletteColor16(const uint16_t idx)
{
    if (m_Palette16 == nullptr) {
        return (RGB_888) { 0, 0, 0 };
    }
    return m_Palette16[idx];
}

LIGHT_MAP *Output_GetLightMap(const uint8_t idx)
{
    return &m_LightMap[idx];
}

SHADE_MAP *Output_GetShadeMap(const uint8_t idx)
{
    return &m_ShadeMap[idx];
}

int32_t Output_GetObjectTextureCount(void)
{
    return m_ObjectTextureCount;
}

OBJECT_TEXTURE *Output_GetObjectTexture(const int32_t texture_idx)
{
    if (m_ObjectTextures == nullptr) {
        return nullptr;
    }
    return &m_ObjectTextures[texture_idx];
}

SPRITE_TEXTURE *Output_GetSpriteTexture(const int32_t texture_idx)
{
    if (m_SpriteTextures == nullptr) {
        return nullptr;
    }
    return &m_SpriteTextures[texture_idx];
}

ANIMATED_TEXTURE_RANGE *Output_GetAnimatedTextureRange(const int32_t range_idx)
{
    if (m_AnimTextureRanges == nullptr) {
        return nullptr;
    }
    return &m_AnimTextureRanges[range_idx];
}

RGBA_8888 Output_RGB2RGBA(const RGB_888 color)
{
    RGBA_8888 ret = { .r = color.r, .g = color.g, .b = color.b, .a = 255 };
    return ret;
}

int16_t Output_FindColor8(const RGB_888 color)
{
    if (m_Palette8 == nullptr) {
        return -1;
    }

    int32_t best_idx = 0;
    int32_t best_diff = INT32_MAX;
    for (int32_t i = 0; i < m_PaletteSize; i++) {
        const int32_t dr = color.r - m_Palette8[i].r;
        const int32_t dg = color.g - m_Palette8[i].g;
        const int32_t db = color.b - m_Palette8[i].b;
        const int32_t diff = SQUARE(dr) + SQUARE(dg) + SQUARE(db);
        if (diff < best_diff) {
            best_diff = diff;
            best_idx = i;
        }
    }

    return best_idx;
}

void Output_CycleAnimatedTextures(void)
{
    const ANIMATED_TEXTURE_RANGE *range = m_AnimTextureRanges;
    for (; range != nullptr; range = range->next_range) {
        int32_t i = 0;
        const OBJECT_TEXTURE temp = m_ObjectTextures[range->textures[i]];
        for (; i < range->num_textures - 1; i++) {
            m_ObjectTextures[range->textures[i]] =
                m_ObjectTextures[range->textures[i + 1]];
        }
        m_ObjectTextures[range->textures[i]] = temp;
    }

    for (int32_t i = 0; i < MAX_STATIC_OBJECTS; i++) {
        const STATIC_OBJECT_2D *const obj = Object_Get2DStatic(i);
        if (!obj->loaded || obj->frame_count == 1) {
            continue;
        }

        const int16_t frame_count = obj->frame_count;
        const SPRITE_TEXTURE temp = m_SpriteTextures[obj->texture_idx];
        for (int32_t j = 0; j < frame_count - 1; j++) {
            m_SpriteTextures[obj->texture_idx + j] =
                m_SpriteTextures[obj->texture_idx + j + 1];
        }
        m_SpriteTextures[obj->texture_idx + frame_count - 1] = temp;
    }
}
