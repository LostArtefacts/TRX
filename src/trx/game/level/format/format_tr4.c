#include <trx/core/benchmark.h>
#include <trx/core/file.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
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

static TRX_FILE *M_ReadChunk(TRX_FILE *const file, const char *const name)
{
    const uint32_t uncompressed_size = File_ReadU32(file);
    const uint32_t compressed_size = File_ReadU32(file);

    size_t compressed_left;
    const char *const compressed = File_PeekBytes(file, &compressed_left);

    char *const payload = Memory_Alloc(uncompressed_size);
    uLongf out_size = uncompressed_size;
    const int32_t error = uncompress(
        (Bytef *)payload, &out_size, (const Bytef *)compressed,
        compressed_size);
    if (error != Z_OK || out_size != uncompressed_size) {
        LOG_ERROR("Failed to inflate TR4 chunk %s", name);
        Memory_Free(payload);
        return nullptr;
    }
    File_Skip(file, compressed_size);

    TRX_FILE *const result = File_OpenBuffer(payload, uncompressed_size);
    Memory_Free(payload);
    return result;
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

static RESULT M_ReadImages(LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    M_IMAGE_META image_meta = {
        .room_pages = File_ReadU16(file),
        .object_pages = File_ReadU16(file),
        .bump_pages = File_ReadU16(file),
    };
    const int32_t page_count =
        image_meta.room_pages + image_meta.object_pages + image_meta.bump_pages;

    TRX_FILE *const images32 = M_ReadChunk(file, "images32");
    if (images32 == nullptr) {
        return FAIL("the 32-bit images could not be read");
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
    File_ReadData(
        images32, info->textures.pages_32,
        sizeof(RGBA_8888) * page_count * TEXTURE_PAGE_SIZE);
    File_Close(images32);

    const int32_t pixel_count = page_count * TEXTURE_PAGE_SIZE;
    for (int32_t i = 0; i < pixel_count; i++) {
        SWAP(info->textures.pages_32[i].r, info->textures.pages_32[i].b);
    }

    TRX_FILE *const images16 = M_ReadChunk(file, "images16");
    if (images16 == nullptr) {
        return FAIL("the 16-bit images could not be read");
    }
    File_Close(images16);

    TRX_FILE *const sky_font = M_ReadChunk(file, "sky/font");
    if (sky_font == nullptr) {
        return FAIL("the sky and font images could not be read");
    }
    // The chunk holds two raw 256x256 BGRA images: the font, then the sky.
    // The sky becomes an extra texture page for the flat sky layers.
    File_Skip(sky_font, sizeof(RGBA_8888) * TEXTURE_PAGE_SIZE);
    const int32_t sky_page = info->textures.page_count;
    RGBA_8888 *const sky_pixels =
        &info->textures.pages_32[sky_page * TEXTURE_PAGE_SIZE];
    File_ReadData(sky_font, sky_pixels, sizeof(RGBA_8888) * TEXTURE_PAGE_SIZE);
    for (int32_t i = 0; i < TEXTURE_PAGE_SIZE; i++) {
        SWAP(sky_pixels[i].r, sky_pixels[i].b);
        sky_pixels[i].a = 255;
    }
    info->textures.page_count++;
    Output_Sky_SetTexturePage(sky_page);
    File_Close(sky_font);

    M_InitialiseDummyPalette(ctx);
    return OK;
}

static void M_ReadAnimatedTextureRangesTR4(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
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
        range->num_textures = File_ReadCountS16(file) + 1;
        range->textures = GameBuf_Alloc(
            sizeof(int16_t) * range->num_textures,
            GBUF_ANIMATED_TEXTURE_RANGES);
        File_ReadData(
            file, range->textures, sizeof(int16_t) * range->num_textures);
    }
    File_Seek(file, end_position, FILE_SEEK_SET);
    // The first N ranges scroll their V linearly (UV rotate) instead of
    // frame-swapping.
    const uint8_t num_uv_rotate_ranges = File_ReadU8(file);
    LOG_INFO("uv rotate ranges: %d", num_uv_rotate_ranges);
    Output_SetUVRotateRangeCount(MIN(num_uv_rotate_ranges, num_ranges));
}

static void M_ReadObjectTexturesTR4(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    char signature[3];
    File_ReadData(file, signature, sizeof(signature));
    if (signature[0] != 'T' || signature[1] != 'E' || signature[2] != 'X') {
        LOG_WARNING("Unexpected TR4 object texture signature");
    }

    const int32_t num_textures = File_ReadCountS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->textures.object_count = num_textures;
    LOG_INFO("object textures: %d", num_textures);
    Output_InitialiseObjectTextures(
        num_textures + Inject_GetDataCount(IDT_OBJECT_TEXTURES));
    Level_Section_AppendObjectTextures(0, 0, num_textures, file);
}

static void M_ReadSpriteTexturesTR4(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    char signature[3];
    File_ReadData(file, signature, sizeof(signature));
    if (signature[0] != 'S' || signature[1] != 'P' || signature[2] != 'R') {
        LOG_WARNING("Unexpected TR4 sprite texture signature");
    }

    const int32_t num_textures = File_ReadCountS32(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->textures.sprite_count = num_textures;
    LOG_INFO("sprite textures: %d", num_textures);
    Output_InitialiseSpriteTextures(
        num_textures + Inject_GetDataCount(IDT_SPRITE_TEXTURES));
    Level_Section_AppendSpriteTextures(0, 0, num_textures, file);

    // TR4 stores the authoritative atlas rect in the corner fields (in
    // pixels); the offset/width/height fields hold stale pre-repack values.
    // Normalize to the TR1-3 convention the rest of the engine consumes.
    // Injected sprites are appended later and are already authored this way.
    for (int32_t i = 0; i < num_textures; i++) {
        SPRITE_TEXTURE *const sprite = Output_GetSpriteTexture(i);
        const int32_t x = sprite->x0;
        const int32_t y = sprite->y0;
        const int32_t w = sprite->x1 - sprite->x0;
        const int32_t h = sprite->y1 - sprite->y0;
        sprite->offset = x | (y << 8);
        sprite->width = w * 256 - 1;
        sprite->height = h * 256 - 1;
        // The corner fields double as billboard extents downstream; TR4
        // files don't author those, so use a centered box.
        sprite->x0 = -w / 2;
        sprite->y0 = -h / 2;
        sprite->x1 = w / 2;
        sprite->y1 = h / 2;
    }
}

static RESULT M_ReadItemsTR4(LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    RESULT result = OK;
    BENCHMARK benchmark = Benchmark_Start();
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    const int32_t num_items = File_ReadCountS32(file);
    LOG_INFO("items: %d", num_items);
    if (num_items > MAX_ITEMS) {
        result =
            FAIL("too many items: %d, at most %d fit", num_items, MAX_ITEMS);
        goto finish;
    }

    info->tr4.item_count = num_items;
    info->tr4.items =
        GameBuf_Alloc(sizeof(LEVEL_TR4_ITEM_INFO) * num_items, GBUF_ITEMS);
    Item_InitialiseItems(num_items);
    for (int32_t i = 0; i < num_items; i++) {
        ITEM *const item = Item_Get(i);
        LEVEL_TR4_ITEM_INFO *const tr4_item = &info->tr4.items[i];
        const int16_t obj_id = File_ReadS16(file);
        tr4_item->object_id = obj_id;
        item->object_id = Object_FromGameID(obj_id);
        tr4_item->room_num = File_ReadS16(file);
        item->room_num = tr4_item->room_num;
        tr4_item->pos.x = File_ReadS32(file);
        tr4_item->pos.y = File_ReadS32(file);
        tr4_item->pos.z = File_ReadS32(file);
        item->pos = tr4_item->pos;
        item->rot.x = 0;
        tr4_item->y_rot = File_ReadS16(file);
        item->rot.y = tr4_item->y_rot;
        item->rot.z = 0;
        const int16_t shade = File_ReadS16(file);
        item->shade.value_1 = shade;
        item->shade.value_2 = shade;
        tr4_item->ocb = File_ReadS16(file);
        tr4_item->flags = File_ReadU16(file);
        item->init_flags = tr4_item->flags;
        if (item->object_id == NO_OBJECT) {
            LOG_WARNING("Unsupported TR4 item object %d on item %d", obj_id, i);
            item->object_id = O_CAMERA_TARGET;
        }
    }

finish:
    Benchmark_End(&benchmark, nullptr);
    return result;
}

static void M_ReadAIItemsTR4(LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    const int32_t num_ai_items = File_ReadCountS32(file);
    info->tr4.ai_item_count = num_ai_items;
    info->tr4.ai_items = GameBuf_Alloc(
        sizeof(LEVEL_TR4_AI_ITEM_INFO) * num_ai_items, GBUF_ITEMS);
    for (int32_t i = 0; i < num_ai_items; i++) {
        LEVEL_TR4_AI_ITEM_INFO *const ai_item = &info->tr4.ai_items[i];
        ai_item->object_id = File_ReadS16(file);
        ai_item->room_num = File_ReadS16(file);
        ai_item->pos.x = File_ReadS32(file);
        ai_item->pos.y = File_ReadS32(file);
        ai_item->pos.z = File_ReadS32(file);
        ai_item->ocb = File_ReadS16(file);
        ai_item->flags = File_ReadU16(file);
        ai_item->y_rot = File_ReadS16(file);
        ai_item->box_num = File_ReadS16(file);
    }
}

static void M_ReadSampleData(LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    const int32_t num_samples = File_ReadCountS32(file);
    ASSERT(info->samples.offset_count == num_samples);

    const size_t start_pos = File_Pos(file);
    int32_t data_size = 0;
    for (int32_t i = 0; i < num_samples; i++) {
        File_Skip(file, sizeof(int32_t)); // unused inflated size
        const int32_t sample_size = File_ReadS32(file);
        info->samples.offsets[i] = data_size;
        data_size += sample_size;
        File_Skip(file, sample_size);
    }

    LOG_INFO("%d sample data size", data_size);
    info->samples.data_size = data_size;
    info->samples.data = GameBuf_Alloc(
        data_size + Inject_GetDataCount(IDT_SAMPLE_DATA), GBUF_SAMPLES);

    File_Seek(file, start_pos, FILE_SEEK_SET);
    char *data = info->samples.data;
    for (int32_t i = 0; i < num_samples; i++) {
        File_Skip(file, sizeof(int32_t));
        const int32_t sample_size = File_ReadS32(file);
        File_ReadData(file, data, sizeof(char) * sample_size);
        data += sample_size;
    }
}

static RESULT M_ProbeImages(TRX_FILE *const file)
{
    const char *const chunk_names[] = {
        "images32",
        "images16",
        "sky/font",
    };

    LEVEL_FORMAT_SKIP_OR_FAIL(6); // page counts
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(chunk_names); i++) {
        TRX_FILE *const images = M_ReadChunk(file, chunk_names[i]);
        if (images == nullptr) {
            return ERR;
        }
        File_Close(images);
    }
    return OK;
}

static RESULT M_ProbeLevelChunk(
    const LEVEL_FORMAT_LOADER *const loader, TRX_FILE *const file,
    const LEVEL_FORMAT_PROBE_MODE mode)
{
    LEVEL_CONTEXT probe_ctx = {
        .loader = loader,
    };
    LEVEL_FORMAT_SKIP_OR_FAIL(4); // unused version number

    MUST(Level_Section_ReadRooms(&probe_ctx, file));

    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(2); // object meshes
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(4); // object mesh pointers
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(40); // animations
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(6); // animation changes
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(8); // animation ranges
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(2); // animation commands
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(4); // animation bones
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(2); // animation frames

    MUST(Level_Section_ReadObjects(&probe_ctx, file));

    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(32); // static objects
    LEVEL_FORMAT_SKIP_OR_FAIL(3); // SPR marker
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(16); // sprite textures
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(8); // sprites sequences
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(16); // cameras/sinks
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(40); // flybys
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(16); // sound sources

    int32_t box_count;
    LEVEL_FORMAT_TRY_OR_FAIL(File_TryReadS32(file, &box_count));
    LEVEL_FORMAT_SKIP_OR_FAIL(box_count * 8);
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(2); // overlaps
    LEVEL_FORMAT_SKIP_OR_FAIL(box_count * 20); // zones

    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(2); // animated texture ranges
    LEVEL_FORMAT_SKIP_OR_FAIL(1); // uv rotate count
    LEVEL_FORMAT_SKIP_OR_FAIL(3); // TEX marker
    LEVEL_FORMAT_SKIP_ARR_S32_OR_FAIL(38); // object textures

    MUST(M_ReadItemsTR4(&probe_ctx, file));

    return OK;
}

static RESULT M_Probe(
    const LEVEL_FORMAT_LOADER *const loader, TRX_FILE *const file,
    const LEVEL_FORMAT_PROBE_MODE mode)
{
    File_Seek(file, 0, FILE_SEEK_SET);
    uint32_t version;
    LEVEL_FORMAT_TRY_OR_FAIL(File_TryReadU32(file, &version));
    if (version != M_VERSION_TR45) {
        return ERR;
    }

    if (mode == LEVEL_FORMAT_PROBE_MINIMAL) {
        // TODO: once TR4X level format is in place, detect this as a separate
        // chunk at the end of the file. For now, minimal probes need only the
        // version number.
        return OK;
    }

    MUST(M_ProbeImages(file));

    TRX_FILE *const level_data = M_ReadChunk(file, "level data");
    if (level_data == nullptr) {
        return ERR;
    }

    const RESULT result = M_ProbeLevelChunk(loader, level_data, mode);
    File_Close(level_data);
    return result;
}

static RESULT M_Load(
    const LEVEL_FORMAT_LOADER *const loader, TRX_FILE *const file)
{
    LEVEL_CONTEXT *const ctx = Level_Context_Get();
    File_Seek(file, sizeof(uint32_t), FILE_SEEK_SET);

    MUST(M_ReadImages(ctx, file));
    TRX_FILE *const level_data = M_ReadChunk(file, "level data");
    if (level_data == nullptr) {
        return FAIL("the level data could not be read");
    }

    File_ReadU32(level_data); // level number
    MUST(Level_Section_ReadRooms(ctx, level_data));
    MUST(Level_Section_ReadObjectMeshes(ctx, level_data));
    MUST(Level_Section_ReadAnims(ctx, level_data));
    MUST(Level_Section_ReadAnimChanges(ctx, level_data));
    MUST(Level_Section_ReadAnimRanges(ctx, level_data));
    MUST(Level_Section_ReadAnimCommands(ctx, level_data));
    MUST(Level_Section_ReadAnimBones(ctx, level_data));
    MUST(Level_Section_ReadAnimFrames(ctx, level_data));
    MUST(Level_Section_ReadObjects(ctx, level_data));
    MUST(Level_Section_ReadStaticObjects(ctx, level_data));
    M_ReadSpriteTexturesTR4(ctx, level_data);
    MUST(Level_Section_ReadSpriteSequences(ctx, level_data));
    MUST(Level_Section_ReadCamerasAndSinks(ctx, level_data));
    MUST(Level_Section_ReadFlybyCameras(ctx, level_data));
    MUST(Level_Section_ReadSoundSources(ctx, level_data));
    MUST(Level_Section_ReadPathingData(ctx, level_data));
    M_ReadAnimatedTextureRangesTR4(ctx, level_data);
    M_ReadObjectTexturesTR4(ctx, level_data);
    MUST(M_ReadItemsTR4(ctx, level_data));
    M_ReadAIItemsTR4(ctx, level_data);
    MUST(Level_Section_ReadDemoData(ctx, level_data));
    MUST(Level_Section_ReadSamples(ctx, level_data));
    File_Skip(level_data, 6); // trailing zero padding

    File_Close(level_data);

    M_ReadSampleData(ctx, file);
    return OK;
}

static const LEVEL_FORMAT_LOADER m_LevelLoaderTR4 = {
    .game_version = 4,
    .layout = LEVEL_FORMAT_LAYOUT_TR4,
    .probe = M_Probe,
    .load = M_Load,
};

REGISTER_LEVEL_FORMAT_LOADER(320, m_LevelLoaderTR4)
