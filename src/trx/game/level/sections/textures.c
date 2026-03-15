#include <trx/core/benchmark.h>
#include <trx/core/colors.h>
#include <trx/core/hash.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/game/game_buf.h>
#include <trx/game/game_flow.h>
#include <trx/game/inject.h>
#include <trx/game/level/cache.h>
#include <trx/game/level/format/format.h>
#include <trx/game/level/sections/append.h>
#include <trx/game/level/sections/read.h>
#include <trx/game/output.h>

#define M_TEXTURE_PAGE_CACHE_VERSION 1

static void M_DecodeTR3ObjectTextureUVs(OBJECT_TEXTURE *const texture)
{
    int16_t *const uv = (int16_t *)&texture->uv[0].u;
    uint8_t flags = 0;

    for (int32_t i = 0; i < 8; i++) {
        if ((uv[i] & 0x80) != 0) {
            uv[i] |= 0x00FF;
            flags |= 1 << i;
        } else {
            uv[i] &= 0xFF00;
        }
    }

    for (int32_t i = 0; i < 8; i++) {
        if ((flags & 1) != 0) {
            uv[i] -= 256;
        } else {
            uv[i] += 256;
        }
        flags >>= 1;
    }
}

static uint64_t M_ComputeExpandedTexturePagesChecksum(
    const LEVEL_FORMAT_LOADER *const loader, const int32_t num_pages)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    if (level == nullptr) {
        return 0;
    }

    uint64_t checksum = LevelCache_InitChecksum(
        "expanded_texture_pages", M_TEXTURE_PAGE_CACHE_VERSION);
    checksum = LevelCache_UpdateLevelChecksum(checksum, level);
    checksum = Hash_FNV1a64_UpdateU32(checksum, loader->game_version);
    checksum = Hash_FNV1a64_UpdateU32(checksum, num_pages);
    return checksum;
}

static const char *M_GetExpandedTexturePagesCacheFilename(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    const char *const level_key = LevelCache_GetLevelKey(level);
    if (level_key == nullptr) {
        return nullptr;
    }

    return String_FormatStatic(
        "expanded_texture_pages_%s.cache.dat", level_key);
}

static bool M_TryLoadExpandedTexturePagesCache(
    const LEVEL_FORMAT_LOADER *const loader, const int32_t num_pages,
    RGBA_8888 *const output)
{
    const char *const cache_filename = M_GetExpandedTexturePagesCacheFilename();
    const uint64_t checksum =
        M_ComputeExpandedTexturePagesChecksum(loader, num_pages);
    if (cache_filename == nullptr || checksum == 0) {
        return false;
    }

    MYFILE *const file = LevelCache_OpenBinaryRead(cache_filename, checksum);
    if (file == nullptr) {
        return false;
    }

    const bool ok = File_ReadData(
        file, output, num_pages * TEXTURE_PAGE_SIZE * sizeof(*output));
    File_Close(file);
    return ok;
}

static void M_WriteExpandedTexturePagesCache(
    const LEVEL_FORMAT_LOADER *const loader, const int32_t num_pages,
    const RGBA_8888 *const output)
{
    const char *const cache_filename = M_GetExpandedTexturePagesCacheFilename();
    const uint64_t checksum =
        M_ComputeExpandedTexturePagesChecksum(loader, num_pages);
    if (cache_filename == nullptr || checksum == 0) {
        return;
    }

    MYFILE *const file = LevelCache_OpenBinaryWrite(cache_filename, checksum);
    if (file == nullptr) {
        return;
    }

    File_WriteData(
        file, output, num_pages * TEXTURE_PAGE_SIZE * sizeof(*output));
    File_Close(file);
}

void Level_Section_ReadPalettes(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();

    const int32_t palette_size = 256;
    const LEVEL_FORMAT_LOADER *const loader = ctx->loader;
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->palette.size = palette_size;

    info->palette.data_24 = Memory_Alloc(sizeof(RGB_888) * palette_size);
    VFile_Read(file, info->palette.data_24, sizeof(RGB_888) * palette_size);
    info->palette.data_24[0].r = 0;
    info->palette.data_24[0].g = 0;
    info->palette.data_24[0].b = 0;
    for (int32_t i = 1; i < palette_size; i++) {
        RGB_888 *const col = &info->palette.data_24[i];
        col->r = (col->r << 2) | (col->r >> 4);
        col->g = (col->g << 2) | (col->g >> 4);
        col->b = (col->b << 2) | (col->b >> 4);
    }

    if (loader->game_version == 1) {
        info->palette.data_32 = nullptr;
    } else {
        RGBA_8888 palette_16[palette_size];
        info->palette.data_32 = Memory_Alloc(sizeof(RGB_888) * palette_size);
        VFile_Read(file, palette_16, sizeof(RGBA_8888) * palette_size);
        for (int32_t i = 0; i < palette_size; i++) {
            info->palette.data_32[i].r = palette_16[i].r;
            info->palette.data_32[i].g = palette_16[i].g;
            info->palette.data_32[i].b = palette_16[i].b;
        }
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_Section_ReadTexturePages(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();

    const int32_t num_pages = VFile_ReadS32(file);
    const LEVEL_FORMAT_LOADER *const loader = ctx->loader;
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->textures.page_count = num_pages;
    LOG_INFO("texture pages: %d", num_pages);

    const int32_t extra_pages = Inject_GetDataCount(IDT_TEXTURE_PAGES);
    const int32_t texture_size_8_bit =
        (num_pages + extra_pages) * TEXTURE_PAGE_SIZE * sizeof(uint8_t);
    const int32_t texture_size_32_bit =
        (num_pages + extra_pages) * TEXTURE_PAGE_SIZE * sizeof(RGBA_8888);

    info->textures.pages_8 = Memory_Alloc(texture_size_8_bit);
    VFile_Read(file, info->textures.pages_8, num_pages * TEXTURE_PAGE_SIZE);

    info->textures.pages_32 = Memory_Alloc(texture_size_32_bit);
    RGBA_8888 *output = info->textures.pages_32;

    if (loader->game_version == 1) {
        const uint8_t *input = info->textures.pages_8;
        for (int32_t i = 0; i < num_pages * TEXTURE_PAGE_SIZE; i++) {
            const uint8_t index = *input++;
            const RGB_888 pix = info->palette.data_24[index];
            output->r = pix.r;
            output->g = pix.g;
            output->b = pix.b;
            output->a = index == 0 ? 0 : 0xFF;
            output++;
        }
    } else {
        const int32_t texture_size_16_bit =
            num_pages * TEXTURE_PAGE_SIZE * sizeof(uint16_t);
        if (M_TryLoadExpandedTexturePagesCache(loader, num_pages, output)) {
            VFile_Skip(file, texture_size_16_bit);
        } else {
            uint16_t *input = Memory_Alloc(texture_size_16_bit);
            uint16_t *input_ptr = input;
            VFile_Read(file, input, texture_size_16_bit);
            for (int32_t i = 0; i < num_pages * TEXTURE_PAGE_SIZE; i++) {
                *output++ = Color_ARGB1555ToRGBA8888(*input_ptr++);
            }
            M_WriteExpandedTexturePagesCache(
                loader, num_pages, info->textures.pages_32);
            Memory_FreePointer(&input);
        }
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_Section_ReadObjectTextures(
    LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_textures = VFile_ReadS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->textures.object_count = num_textures;
    LOG_INFO("object textures: %d", num_textures);
    Output_InitialiseObjectTextures(
        num_textures + Inject_GetDataCount(IDT_OBJECT_TEXTURES));
    Level_Section_AppendObjectTextures(
        0, 0, num_textures, file, ctx->loader->game_version >= 3);
    Benchmark_End(&benchmark, nullptr);
}

void Level_Section_AppendObjectTextures(
    const int32_t base_idx, const int16_t base_page_idx,
    const int32_t num_textures, VFILE *const file,
    const bool use_tr3_adjustment)
{
    for (int32_t i = 0; i < num_textures; i++) {
        OBJECT_TEXTURE *const texture = Output_GetObjectTexture(base_idx + i);
        texture->uv_count = 4; // Default to 4 vertices
        texture->draw_type = VFile_ReadU16(file);
        texture->tex_page = VFile_ReadU16(file) + base_page_idx;
        for (int32_t j = 0; j < 4; j++) {
            texture->uv[j].u = VFile_ReadU16(file);
            texture->uv[j].v = VFile_ReadU16(file);
        }
        if (use_tr3_adjustment) {
            M_DecodeTR3ObjectTextureUVs(texture);
        }
    }
}

void Level_Section_ReadSpriteTextures(
    LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_textures = VFile_ReadS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->textures.sprite_count = num_textures;
    LOG_INFO("sprite textures: %d", num_textures);
    Output_InitialiseSpriteTextures(
        num_textures + Inject_GetDataCount(IDT_SPRITE_TEXTURES));
    Level_Section_AppendSpriteTextures(0, 0, num_textures, file);

    Benchmark_End(&benchmark, nullptr);
}

void Level_Section_AppendSpriteTextures(
    const int32_t base_idx, const int16_t base_page_idx,
    const int32_t num_textures, VFILE *const file)
{
    for (int32_t i = 0; i < num_textures; i++) {
        SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(base_idx + i);
        sprite->tex_page = VFile_ReadU16(file) + base_page_idx;
        sprite->offset = VFile_ReadU16(file);
        sprite->width = VFile_ReadU16(file);
        sprite->height = VFile_ReadU16(file);
        sprite->x0 = VFile_ReadS16(file);
        sprite->y0 = VFile_ReadS16(file);
        sprite->x1 = VFile_ReadS16(file);
        sprite->y1 = VFile_ReadS16(file);
    }
}

void Level_Section_ReadAnimatedTextureRanges(
    LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t data_size = VFile_ReadS32(file);
    const size_t end_position =
        VFile_GetPos(file) + data_size * sizeof(int16_t);

    const int16_t num_ranges = VFile_ReadS16(file);
    LOG_INFO("animated texture ranges: %d", num_ranges);
    Output_InitialiseAnimatedTextures(num_ranges);

    for (int32_t i = 0; i < num_ranges; i++) {
        ANIMATED_TEXTURE_RANGE *const range = Output_GetAnimatedTextureRange(i);
        range->next_range = i == num_ranges - 1
            ? nullptr
            : Output_GetAnimatedTextureRange(i + 1);

        // Level data is tied to the original logic in Output_AnimateTextures
        // and hence stores one less than the actual count here.
        range->num_textures = VFile_ReadS16(file) + 1;
        range->textures = GameBuf_Alloc(
            sizeof(int16_t) * range->num_textures,
            GBUF_ANIMATED_TEXTURE_RANGES);
        VFile_Read(
            file, range->textures, sizeof(int16_t) * range->num_textures);
    }

    VFile_SetPos(file, end_position);
    Benchmark_End(&benchmark, nullptr);
}

void Level_Section_ReadLightMap(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    for (int32_t i = 0; i < 32; i++) {
        LIGHT_MAP *const light_map = Output_GetLightMap(i);
        VFile_Read(file, light_map->index, sizeof(uint8_t) * 256);
        light_map->index[0] = 0;
    }

    for (int32_t i = 0; i < 32; i++) {
        const LIGHT_MAP *const light_map = Output_GetLightMap(i);
        for (int32_t j = 0; j < 256; j++) {
            SHADE_MAP *const shade_map = Output_GetShadeMap(j);
            shade_map->index[i] = light_map->index[j];
        }
    }

    Benchmark_End(&benchmark, nullptr);
}
