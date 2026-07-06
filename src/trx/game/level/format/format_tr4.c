#include <trx/core/benchmark.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/core/virtual_file.h>
#include <trx/debug.h>
#include <trx/game/const.h>
#include <trx/game/game_buf.h>
#include <trx/game/inject.h>
#include <trx/game/items.h>
#include <trx/game/level/context.h>
#include <trx/game/level/format/priv.h>
#include <trx/game/level/sections/append.h>
#include <trx/game/level/sections/read.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/shell.h>

#include <string.h>
#include <zlib.h>

#define M_VERSION_TR45 0x00345254u

typedef struct {
    uint16_t room_pages;
    uint16_t object_pages;
    uint16_t bump_pages;
} M_IMAGE_META;

static VFILE *M_ReadChunk(VFILE *const file, const char *const name)
{
    const uint32_t uncompressed_size = VFile_ReadU32(file);
    const uint32_t compressed_size = VFile_ReadU32(file);

    char *const payload = Memory_Alloc(uncompressed_size);
    uLongf out_size = uncompressed_size;
    const int32_t error = uncompress(
        (Bytef *)payload, &out_size, (const Bytef *)file->cur_ptr,
        compressed_size);
    if (error != Z_OK || out_size != uncompressed_size) {
        LOG_ERROR("Failed to inflate TR4 chunk %s", name);
        Memory_Free(payload);
        return nullptr;
    }
    VFile_Skip(file, compressed_size);

    VFILE *const result = VFile_CreateFromBuffer(payload, uncompressed_size);
    Memory_Free(payload);
    return result;
}

static bool M_Probe(
    const LEVEL_FORMAT_LOADER *const, VFILE *const file,
    const LEVEL_FORMAT_PROBE_MODE)
{
    VFile_SetPos(file, 0);
    uint32_t version;
    LEVEL_FORMAT_TRY_OR_FAIL(VFile_TryReadU32(file, &version));
    return version == M_VERSION_TR45;
}

static void M_InitialiseDummyPalette(LEVEL_CONTEXT *const ctx)
{
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->palette.size = 256;
    info->palette.data_24 = Memory_Alloc(sizeof(RGB_888) * info->palette.size);
    info->palette.data_32 = Memory_Alloc(sizeof(RGB_888) * info->palette.size);
    memset(info->palette.data_24, 0, sizeof(RGB_888) * info->palette.size);
    memset(info->palette.data_32, 0, sizeof(RGB_888) * info->palette.size);
}

static bool M_ReadImages(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    M_IMAGE_META image_meta = {
        .room_pages = VFile_ReadU16(file),
        .object_pages = VFile_ReadU16(file),
        .bump_pages = VFile_ReadU16(file),
    };
    const int32_t page_count =
        image_meta.room_pages + image_meta.object_pages + image_meta.bump_pages;

    VFILE *const images32 = M_ReadChunk(file, "images32");
    if (images32 == nullptr) {
        Shell_ExitSystem("Failed to read TR4 32-bit images");
        return false;
    }

    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->textures.page_count = page_count;
    // One extra page for the sky image from the "sky/font" chunk.
    const int32_t alloc_page_count =
        page_count + 1 + Inject_GetDataCount(IDT_TEXTURE_PAGES);
    info->textures.pages_8 =
        Memory_Alloc(sizeof(uint8_t) * alloc_page_count * TEXTURE_PAGE_SIZE);
    info->textures.pages_32 =
        Memory_Alloc(sizeof(RGBA_8888) * alloc_page_count * TEXTURE_PAGE_SIZE);
    memset(info->textures.pages_8, 0, alloc_page_count * TEXTURE_PAGE_SIZE);
    memset(
        info->textures.pages_32, 0,
        sizeof(RGBA_8888) * alloc_page_count * TEXTURE_PAGE_SIZE);
    VFile_Read(
        images32, info->textures.pages_32,
        sizeof(RGBA_8888) * page_count * TEXTURE_PAGE_SIZE);
    VFile_Close(images32);

    const int32_t pixel_count = page_count * TEXTURE_PAGE_SIZE;
    for (int32_t i = 0; i < pixel_count; i++) {
        SWAP(info->textures.pages_32[i].r, info->textures.pages_32[i].b);
    }

    VFILE *const images16 = M_ReadChunk(file, "images16");
    if (images16 == nullptr) {
        Shell_ExitSystem("Failed to read TR4 16-bit images");
        return false;
    }
    VFile_Close(images16);

    VFILE *const sky_font = M_ReadChunk(file, "sky/font");
    if (sky_font == nullptr) {
        Shell_ExitSystem("Failed to read TR4 sky/font images");
        return false;
    }
    // The chunk holds two raw 256x256 BGRA images: the font, then the sky.
    // The sky becomes an extra texture page for the flat sky layers.
    VFile_Skip(sky_font, sizeof(RGBA_8888) * TEXTURE_PAGE_SIZE);
    const int32_t sky_page = info->textures.page_count;
    RGBA_8888 *const sky_pixels =
        &info->textures.pages_32[sky_page * TEXTURE_PAGE_SIZE];
    VFile_Read(sky_font, sky_pixels, sizeof(RGBA_8888) * TEXTURE_PAGE_SIZE);
    for (int32_t i = 0; i < TEXTURE_PAGE_SIZE; i++) {
        SWAP(sky_pixels[i].r, sky_pixels[i].b);
        sky_pixels[i].a = 255;
    }
    info->textures.page_count++;
    Output_Sky_SetTexturePage(sky_page);
    VFile_Close(sky_font);

    M_InitialiseDummyPalette(ctx);
    return true;
}

static void M_ReadAnimatedTextureRangesTR4(
    LEVEL_CONTEXT *const ctx, VFILE *const file)
{
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
        range->num_textures = VFile_ReadS16(file) + 1;
        range->textures = GameBuf_Alloc(
            sizeof(int16_t) * range->num_textures,
            GBUF_ANIMATED_TEXTURE_RANGES);
        VFile_Read(
            file, range->textures, sizeof(int16_t) * range->num_textures);
    }
    VFile_SetPos(file, end_position);
    // The first N ranges scroll their V linearly (UV rotate) instead of
    // frame-swapping.
    const uint8_t num_uv_rotate_ranges = VFile_ReadU8(file);
    LOG_INFO("uv rotate ranges: %d", num_uv_rotate_ranges);
    Output_SetUVRotateRangeCount(MIN(num_uv_rotate_ranges, num_ranges));
}

static void M_ReadObjectTexturesTR4(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    char signature[3];
    VFile_Read(file, signature, sizeof(signature));
    if (signature[0] != 'T' || signature[1] != 'E' || signature[2] != 'X') {
        LOG_WARNING("Unexpected TR4 object texture signature");
    }

    const int32_t num_textures = VFile_ReadS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->textures.object_count = num_textures;
    LOG_INFO("object textures: %d", num_textures);
    Output_InitialiseObjectTextures(
        num_textures + Inject_GetDataCount(IDT_OBJECT_TEXTURES));
    Level_Section_AppendObjectTextures(0, 0, num_textures, file);
}

static void M_ReadSpriteTexturesTR4(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    char signature[3];
    VFile_Read(file, signature, sizeof(signature));
    if (signature[0] != 'S' || signature[1] != 'P' || signature[2] != 'R') {
        LOG_WARNING("Unexpected TR4 sprite texture signature");
    }

    const int32_t num_textures = VFile_ReadS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->textures.sprite_count = num_textures;
    LOG_INFO("sprite textures: %d", num_textures);
    Output_InitialiseSpriteTextures(
        num_textures + Inject_GetDataCount(IDT_SPRITE_TEXTURES));
    Level_Section_AppendSpriteTextures(0, 0, num_textures, file);
}

static void M_ReadItemsTR4(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    const int32_t num_items = VFile_ReadS32(file);
    LOG_INFO("items: %d", num_items);
    if (num_items > MAX_ITEMS) {
        Shell_ExitSystem("Too many items");
        Benchmark_End(&benchmark, nullptr);
        return;
    }

    info->tr4.item_count = num_items;
    info->tr4.items =
        GameBuf_Alloc(sizeof(LEVEL_TR4_ITEM_INFO) * num_items, GBUF_ITEMS);
    Item_InitialiseItems(num_items);
    for (int32_t i = 0; i < num_items; i++) {
        ITEM *const item = Item_Get(i);
        LEVEL_TR4_ITEM_INFO *const tr4_item = &info->tr4.items[i];
        const int16_t obj_id = VFile_ReadS16(file);
        tr4_item->object_id = obj_id;
        item->object_id = Object_FromGameID(obj_id);
        tr4_item->room_num = VFile_ReadS16(file);
        item->room_num = tr4_item->room_num;
        tr4_item->pos.x = VFile_ReadS32(file);
        tr4_item->pos.y = VFile_ReadS32(file);
        tr4_item->pos.z = VFile_ReadS32(file);
        item->pos = tr4_item->pos;
        item->rot.x = 0;
        tr4_item->y_rot = VFile_ReadS16(file);
        item->rot.y = tr4_item->y_rot;
        item->rot.z = 0;
        const int16_t shade = VFile_ReadS16(file);
        item->shade.value_1 = shade;
        item->shade.value_2 = shade;
        tr4_item->ocb = VFile_ReadS16(file);
        tr4_item->flags = VFile_ReadU16(file);
        item->flags = tr4_item->flags;
        if (item->object_id == NO_OBJECT) {
            LOG_WARNING("Unsupported TR4 item object %d on item %d", obj_id, i);
        }
    }
    Benchmark_End(&benchmark, nullptr);
}

static void M_ReadAIItemsTR4(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    const int32_t num_ai_items = VFile_ReadS32(file);
    info->tr4.ai_item_count = num_ai_items;
    info->tr4.ai_items = GameBuf_Alloc(
        sizeof(LEVEL_TR4_AI_ITEM_INFO) * num_ai_items, GBUF_ITEMS);
    for (int32_t i = 0; i < num_ai_items; i++) {
        LEVEL_TR4_AI_ITEM_INFO *const ai_item = &info->tr4.ai_items[i];
        ai_item->object_id = VFile_ReadS16(file);
        ai_item->room_num = VFile_ReadS16(file);
        ai_item->pos.x = VFile_ReadS32(file);
        ai_item->pos.y = VFile_ReadS32(file);
        ai_item->pos.z = VFile_ReadS32(file);
        ai_item->ocb = VFile_ReadS16(file);
        ai_item->flags = VFile_ReadU16(file);
        ai_item->y_rot = VFile_ReadS16(file);
        ai_item->box_num = VFile_ReadS16(file);
    }
}

static void M_ReadSampleData(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    const int32_t num_samples = VFile_ReadS32(file);
    ASSERT(info->samples.offset_count == num_samples);

    const size_t start_pos = VFile_GetPos(file);
    int32_t data_size = 0;
    for (int32_t i = 0; i < num_samples; i++) {
        VFile_Skip(file, sizeof(int32_t)); // unused inflated size
        const int32_t sample_size = VFile_ReadS32(file);
        info->samples.offsets[i] = data_size;
        data_size += sample_size;
        VFile_Skip(file, sample_size);
    }

    LOG_INFO("%d sample data size", data_size);
    info->samples.data_size = data_size;
    info->samples.data = GameBuf_Alloc(
        data_size + Inject_GetDataCount(IDT_SAMPLE_DATA), GBUF_SAMPLES);

    VFile_SetPos(file, start_pos);
    char *data = info->samples.data;
    for (int32_t i = 0; i < num_samples; i++) {
        VFile_Skip(file, sizeof(int32_t));
        const int32_t sample_size = VFile_ReadS32(file);
        VFile_Read(file, data, sizeof(char) * sample_size);
        data += sample_size;
    }
}

static bool M_Load(const LEVEL_FORMAT_LOADER *const loader, VFILE *const file)
{
    LEVEL_CONTEXT *const ctx = Level_Context_Get();
    VFile_SetPos(file, sizeof(uint32_t));

    if (!M_ReadImages(ctx, file)) {
        return false;
    }
    VFILE *const level_data = M_ReadChunk(file, "level data");
    if (level_data == nullptr) {
        Shell_ExitSystem("Failed to read TR4 level data");
        return false;
    }

    VFile_ReadU32(level_data); // level number
    Level_Section_ReadRooms(ctx, level_data);
    Level_Section_ReadObjectMeshes(ctx, level_data);
    Level_Section_ReadAnims(ctx, level_data);
    Level_Section_ReadAnimChanges(ctx, level_data);
    Level_Section_ReadAnimRanges(ctx, level_data);
    Level_Section_ReadAnimCommands(ctx, level_data);
    Level_Section_ReadAnimBones(ctx, level_data);
    Level_Section_ReadAnimFrames(ctx, level_data);
    Level_Section_ReadObjects(ctx, level_data);
    Level_Section_ReadStaticObjects(ctx, level_data);
    M_ReadSpriteTexturesTR4(ctx, level_data);
    Level_Section_ReadSpriteSequences(ctx, level_data);
    Level_Section_ReadCamerasAndSinks(ctx, level_data);
    Level_Section_ReadFlybyCameras(ctx, level_data);
    Level_Section_ReadSoundSources(ctx, level_data);
    Level_Section_ReadPathingData(ctx, level_data);
    M_ReadAnimatedTextureRangesTR4(ctx, level_data);
    M_ReadObjectTexturesTR4(ctx, level_data);
    M_ReadItemsTR4(ctx, level_data);
    M_ReadAIItemsTR4(ctx, level_data);
    Level_Section_ReadDemoData(ctx, level_data);
    Level_Section_ReadSamples(ctx, level_data);
    VFile_Skip(level_data, 6); // trailing zero padding

    VFile_Close(level_data);

    M_ReadSampleData(ctx, file);
    return true;
}

static const LEVEL_FORMAT_LOADER m_LevelLoaderTR4 = {
    .game_version = 4,
    .layout = LEVEL_FORMAT_LAYOUT_TR4,
    .probe = M_Probe,
    .load = M_Load,
};

REGISTER_LEVEL_FORMAT_LOADER(320, m_LevelLoaderTR4)
