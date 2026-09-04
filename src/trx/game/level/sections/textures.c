#include <trx/core/benchmark.h>
#include <trx/core/colors.h>
#include <trx/core/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/thread_pool.h>
#include <trx/debug.h>
#include <trx/game/game_buf.h>
#include <trx/game/inject.h>
#include <trx/game/level/format/format.h>
#include <trx/game/level/sections/append.h>
#include <trx/game/level/sections/read.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>

typedef struct {
    const RGB_888 *palette;
    const uint8_t *input_8_page;
    const uint16_t *input_16_page;
    RGBA_8888 *output_32_page;
} M_TEXTURE_PAGE_DECODE_JOB;

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

static void M_Decode8BitTexturePage(void *const userdata)
{
    const M_TEXTURE_PAGE_DECODE_JOB *const job = userdata;
    const uint8_t *input = job->input_8_page;
    RGBA_8888 *output = job->output_32_page;

    for (int32_t i = 0; i < TEXTURE_PAGE_SIZE; i++) {
        const uint8_t index = *input++;
        const RGB_888 pix = job->palette[index];
        output->r = pix.r;
        output->g = pix.g;
        output->b = pix.b;
        output->a = index == 0 ? 0 : 0xFF;
        output++;
    }
}

static void M_Decode16BitTexturePage(void *const userdata)
{
    const M_TEXTURE_PAGE_DECODE_JOB *const job = userdata;
    const uint16_t *input = job->input_16_page;
    RGBA_8888 *output = job->output_32_page;

    for (int32_t i = 0; i < TEXTURE_PAGE_SIZE; i++) {
        *output++ = Color_ARGB1555ToRGBA8888(*input++);
    }
}

// Rooms and object meshes are read before the textures they name, so the
// numbers they hold are checked once the textures are in.
static RESULT M_CheckFaces(
    const char *const owner, const int32_t owner_idx, const FACE *const faces,
    const int32_t count, const int32_t num_textures)
{
    for (int32_t i = 0; i < count; i++) {
        FAIL_IF(
            faces[i].texture_idx >= num_textures,
            "%s %d: face names texture %d of the %d the level holds", owner,
            owner_idx, faces[i].texture_idx, num_textures);
    }
    return OK;
}

static RESULT M_CheckFaceTextures(const int32_t num_textures)
{
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        MUST(M_CheckFaces(
            "room", i, room->mesh.all_faces.data, room->mesh.all_faces.count,
            num_textures));
    }
    for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
        const OBJECT_MESH *const mesh = Object_GetMesh(i);
        MUST(M_CheckFaces(
            "mesh", i, mesh->tex_face4s.data, mesh->tex_face4s.count,
            num_textures));
        MUST(M_CheckFaces(
            "mesh", i, mesh->tex_face3s.data, mesh->tex_face3s.count,
            num_textures));
    }
    return OK;
}

RESULT Level_Section_ReadPalettes(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();

    const int32_t palette_size = 256;
    const LEVEL_FORMAT_LOADER *const loader = ctx->loader;
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->palette.size = palette_size;

    info->palette.data_24 = Memory_Alloc(sizeof(RGB_888) * palette_size);
    File_ReadData(file, info->palette.data_24, sizeof(RGB_888) * palette_size);
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
        File_ReadData(file, palette_16, sizeof(RGBA_8888) * palette_size);
        for (int32_t i = 0; i < palette_size; i++) {
            info->palette.data_32[i].r = palette_16[i].r;
            info->palette.data_32[i].g = palette_16[i].g;
            info->palette.data_32[i].b = palette_16[i].b;
        }
    }

    Benchmark_End(&benchmark, nullptr);
    return OK;
}

RESULT Level_Section_ReadTexturePages(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();

    const int32_t num_pages = File_ReadCountS32(file);
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
    File_ReadData(file, info->textures.pages_8, num_pages * TEXTURE_PAGE_SIZE);

    info->textures.pages_32 = Memory_Alloc(texture_size_32_bit);

    THREAD_POOL *const pool = ThreadPool_Create(-1);
    M_TEXTURE_PAGE_DECODE_JOB *const jobs =
        Memory_Alloc(sizeof(*jobs) * num_pages);
    uint16_t *input_16 = nullptr;
    for (int32_t i = 0; i < num_pages; i++) {
        jobs[i].palette = info->palette.data_24;
        jobs[i].input_8_page = &info->textures.pages_8[i * TEXTURE_PAGE_SIZE];
        jobs[i].input_16_page = nullptr;
        jobs[i].output_32_page =
            &info->textures.pages_32[i * TEXTURE_PAGE_SIZE];
    }

    if (loader->game_version == 1) {
        for (int32_t i = 0; i < num_pages; i++) {
            ThreadPool_AddJob(pool, M_Decode8BitTexturePage, &jobs[i]);
        }
    } else {
        const int32_t texture_size_16_bit =
            num_pages * TEXTURE_PAGE_SIZE * sizeof(uint16_t);
        input_16 = Memory_Alloc(texture_size_16_bit);
        File_ReadData(file, input_16, texture_size_16_bit);
        for (int32_t i = 0; i < num_pages; i++) {
            jobs[i].input_16_page = &input_16[i * TEXTURE_PAGE_SIZE];
            ThreadPool_AddJob(pool, M_Decode16BitTexturePage, &jobs[i]);
        }
    }

    ThreadPool_Wait(pool);
    Memory_FreePointer(&input_16);
    Memory_Free(jobs);
    ThreadPool_Destroy(pool);

    Benchmark_End(&benchmark, nullptr);
    return OK;
}

RESULT Level_Section_ReadObjectTextures(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_textures = File_ReadCountS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->textures.object_count = num_textures;
    LOG_INFO("object textures: %d", num_textures);
    Output_InitialiseObjectTextures(
        num_textures + Inject_GetDataCount(IDT_OBJECT_TEXTURES));
    Level_Section_AppendObjectTextures(0, 0, num_textures, file);

    if (ctx->loader->game_version == 3) {
        for (int32_t i = 0; i < num_textures; i++) {
            OBJECT_TEXTURE *const texture = Output_GetObjectTexture(i);
            M_DecodeTR3ObjectTextureUVs(texture);
        }
    }

    MUST(M_CheckFaceTextures(num_textures));
    Benchmark_End(&benchmark, nullptr);
    return OK;
}

void Level_Section_AppendObjectTextures(
    const int32_t base_idx, const int16_t base_page_idx,
    const int32_t num_textures, TRX_FILE *const file)
{
    const LEVEL_FORMAT_LOADER *const loader = Level_Context_Get()->loader;
    for (int32_t i = 0; i < num_textures; i++) {
        OBJECT_TEXTURE *const texture = Output_GetObjectTexture(base_idx + i);
        texture->draw_type = File_ReadU16(file);
        const uint16_t tex_page = File_ReadU16(file);
        if (loader->game_version == 4) {
            const uint16_t tex_flags = File_ReadU16(file);
            texture->uv_count = ((tex_page | tex_flags) & 0x8000) != 0 ? 3 : 4;
            texture->tex_page = (tex_page & 0x7FFF) + base_page_idx;
        } else {
            texture->uv_count = 4; // Default to 4 vertices
            texture->tex_page = tex_page + base_page_idx;
        }
        for (int32_t j = 0; j < 4; j++) {
            texture->uv[j].u = File_ReadU16(file);
            texture->uv[j].v = File_ReadU16(file);
        }
        if (loader->game_version == 4) {
            File_Skip(file, sizeof(uint32_t) * 4); // x/y offset and dimensions
        }
    }
}

RESULT Level_Section_ReadSpriteTextures(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_textures = File_ReadCountS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->textures.sprite_count = num_textures;
    LOG_INFO("sprite textures: %d", num_textures);
    Output_InitialiseSpriteTextures(
        num_textures + Inject_GetDataCount(IDT_SPRITE_TEXTURES));
    Level_Section_AppendSpriteTextures(0, 0, num_textures, file);

    Benchmark_End(&benchmark, nullptr);
    return OK;
}

void Level_Section_AppendSpriteTextures(
    const int32_t base_idx, const int16_t base_page_idx,
    const int32_t num_textures, TRX_FILE *const file)
{
    ASSERT(base_idx >= 0);
    ASSERT(num_textures >= 0);
    ASSERT(base_idx + num_textures <= Output_GetSpriteTextureCount());
    for (int32_t i = 0; i < num_textures; i++) {
        SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(base_idx + i);
        sprite->tex_page = File_ReadU16(file) + base_page_idx;
        sprite->offset = File_ReadU16(file);
        sprite->width = File_ReadU16(file);
        sprite->height = File_ReadU16(file);
        sprite->x0 = File_ReadS16(file);
        sprite->y0 = File_ReadS16(file);
        sprite->x1 = File_ReadS16(file);
        sprite->y1 = File_ReadS16(file);
    }
}

RESULT Level_Section_ReadAnimatedTextureRanges(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t data_size = File_ReadS32(file);
    const size_t end_position = File_Pos(file) + data_size * sizeof(int16_t);

    const int16_t num_ranges = File_ReadCountS16(file);
    LOG_INFO("animated texture ranges: %d", num_ranges);
    Output_InitialiseAnimatedTextures(num_ranges);

    for (int32_t i = 0; i < num_ranges; i++) {
        ANIMATED_TEXTURE_RANGE *const range = Output_GetAnimatedTextureRange(i);
        range->next_range = i == num_ranges - 1
            ? nullptr
            : Output_GetAnimatedTextureRange(i + 1);

        // Level data is tied to the original logic in Output_AnimateTextures
        // and hence stores one less than the actual count here.
        range->num_textures = File_ReadCountS16(file) + 1;
        range->textures = GameBuf_Alloc(
            sizeof(int16_t) * range->num_textures,
            GBUF_ANIMATED_TEXTURE_RANGES);
        File_ReadData(
            file, range->textures, sizeof(int16_t) * range->num_textures);
    }

    File_Seek(file, end_position, FILE_SEEK_SET);
    Benchmark_End(&benchmark, nullptr);
    return OK;
}

RESULT Level_Section_ReadLightMap(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    for (int32_t i = 0; i < 32; i++) {
        LIGHT_MAP *const light_map = Output_GetLightMap(i);
        File_ReadData(file, light_map->index, sizeof(uint8_t) * 256);
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
    return OK;
}
