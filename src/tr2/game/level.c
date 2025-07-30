#include "game/level.h"

#include "decomp/decomp.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/lara.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/render/common.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/benchmark.h>
#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/filesystem.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/carrier.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/game/gym.h>
#include <libtrx/game/inject.h>
#include <libtrx/game/level.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/game/option.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>
#include <libtrx/virtual_file.h>

#define DEFAULT_SFX_PATH "data/main.sfx"

typedef struct {
    int32_t game_index;
    int32_t file_index;
} SAMPLE_ENTRY;

typedef enum {
    LEVEL_LAYOUT_UNKNOWN = -1,
    LEVEL_LAYOUT_TR2X,
    LEVEL_LAYOUT_TR2,
    LEVEL_LAYOUT_NUMBER_OF,
} M_LAYOUT;

static bool M_TryLayout(VFILE *file, M_LAYOUT layout);
static void M_SkimLevel(VFILE *file, M_LAYOUT layout);
static M_LAYOUT M_GuessLayout(VFILE *file);
static int32_t M_CompareSampleOffsets(const void *a, const void *b);
static void M_LoadFromFile(const GF_LEVEL *level);
static void M_InitialiseSoundEffects(const char *const file_name);
static void M_CompleteSetup(const GF_LEVEL *level);

static bool M_TryLayout(VFILE *const file, const M_LAYOUT layout)
{
    // TODO: clang-format <20 formats this wrongly
    // clang-format off
#define TRY_OR_FAIL(call)                                                      \
    if (!call) {                                                               \
        return false;                                                          \
    }
#define TRY_OR_FAIL_ARR_S32(size)                                              \
    {                                                                          \
        int32_t num;                                                           \
        TRY_OR_FAIL(VFile_TryReadS32(file, &num));                             \
        TRY_OR_FAIL(VFile_TrySkip(file, num * size));                          \
    }
#define TRY_OR_FAIL_ARR_U16(size)                                              \
    {                                                                          \
        uint16_t num;                                                          \
        TRY_OR_FAIL(VFile_TryReadU16(file, &num));                             \
        TRY_OR_FAIL(VFile_TrySkip(file, num * size));                          \
    }
    // clang-format on

    VFile_SetPos(file, 0);

    int32_t version;
    TRY_OR_FAIL(VFile_TryReadS32(file, &version));
    if (version != 45) {
        return false;
    }

    TRY_OR_FAIL(VFile_TrySkip(file, 1792)); // palettes
    TRY_OR_FAIL_ARR_S32(TEXTURE_PAGE_SIZE * 3); // texture pages
    TRY_OR_FAIL(VFile_TrySkip(file, 4)); // unused version number

    uint16_t room_count;
    TRY_OR_FAIL(VFile_TryReadU16(file, &room_count));
    for (int32_t i = 0; i < room_count; i++) {
        TRY_OR_FAIL(VFile_TrySkip(file, 16));
        TRY_OR_FAIL_ARR_S32(2); // meshes
        TRY_OR_FAIL_ARR_U16(32); // portals

        int16_t size_z;
        int16_t size_x;
        TRY_OR_FAIL(VFile_TryReadS16(file, &size_z));
        TRY_OR_FAIL(VFile_TryReadS16(file, &size_x));
        TRY_OR_FAIL(VFile_TrySkip(file, size_z * size_x * 8)); // sectors

        TRY_OR_FAIL(VFile_TrySkip(file, 6)); // lighting
        TRY_OR_FAIL_ARR_U16(24); // lights
        TRY_OR_FAIL_ARR_U16(20); // static meshes
        TRY_OR_FAIL(VFile_TrySkip(file, 4));
    }

    TRY_OR_FAIL_ARR_S32(2); // floor data
    TRY_OR_FAIL_ARR_S32(2); // object meshes
    TRY_OR_FAIL_ARR_S32(4); // object mesh pointers
    TRY_OR_FAIL_ARR_S32(32); // animations
    TRY_OR_FAIL_ARR_S32(6); // animation changes
    TRY_OR_FAIL_ARR_S32(8); // animation ranges
    TRY_OR_FAIL_ARR_S32(2); // animation commands
    TRY_OR_FAIL_ARR_S32(4); // animation bones
    TRY_OR_FAIL_ARR_S32(2); // animation frames
    TRY_OR_FAIL_ARR_S32(18); // objects
    TRY_OR_FAIL_ARR_S32(32); // static objects
    TRY_OR_FAIL_ARR_S32(20); // object textures
    TRY_OR_FAIL_ARR_S32(16); // sprite textures
    TRY_OR_FAIL_ARR_S32(8); // sprites sequences
    TRY_OR_FAIL_ARR_S32(16); // cameras/sinks
    TRY_OR_FAIL_ARR_S32(16); // sound sources

    int32_t box_count;
    TRY_OR_FAIL(VFile_TryReadS32(file, &box_count));
    TRY_OR_FAIL(VFile_TrySkip(file, box_count * 8));
    TRY_OR_FAIL_ARR_S32(2); // overlaps
    TRY_OR_FAIL(VFile_TrySkip(file, box_count * 20)); // zones

    TRY_OR_FAIL_ARR_S32(2); // animated texture ranges
    TRY_OR_FAIL_ARR_S32(24); // items

    TRY_OR_FAIL(VFile_TrySkip(file, 32 * 256)); // light table
    TRY_OR_FAIL_ARR_U16(16); // cinematic frames
    TRY_OR_FAIL_ARR_U16(1); // demo data
    TRY_OR_FAIL(VFile_TrySkip(file, 2 * SFX_NUMBER_OF)); // sample lut
    TRY_OR_FAIL_ARR_S32(8); // sample infos
    TRY_OR_FAIL_ARR_S32(4); // samples

    if (layout == LEVEL_LAYOUT_TR2X) {
        uint32_t inj_magic;
        TRY_OR_FAIL(VFile_TryReadU32(file, &inj_magic));
        TRY_OR_FAIL((inj_magic == INJECTION_MAGIC));
    }

#undef TRY_OR_FAIL
#undef TRY_OR_FAIL_ARR_U16
#undef TRY_OR_FAIL_ARR_S32

    return true;
}

static void M_SkimLevel(VFILE *const file, M_LAYOUT layout)
{
    // TODO: clang-format <20 formats this wrongly
    // clang-format off
#define SKIP_ARR_S32(size)                                                     \
    {                                                                          \
        const int32_t num = VFile_ReadS32(file);                               \
        VFile_Skip(file, num * size);                                          \
    }
#define SKIP_ARR_U16(size)                                                     \
    {                                                                          \
        const uint16_t num = VFile_ReadU16(file);                              \
        VFile_Skip(file, num * size);                                          \
    }
    // clang-format on

    ASSERT(layout != LEVEL_LAYOUT_UNKNOWN);
    VFile_SetPos(file, 4); // start after version number
    VFile_Skip(file, 1792); // palettes
    SKIP_ARR_S32(TEXTURE_PAGE_SIZE * 3); // texture pages
    VFile_Skip(file, 4); // unused version number

    const uint16_t room_count = VFile_ReadU16(file);
    for (int32_t i = 0; i < room_count; i++) {
        VFile_Skip(file, 16);
        SKIP_ARR_S32(2); // meshes
        SKIP_ARR_U16(32); // portals

        const int16_t size_z = VFile_ReadS16(file);
        const int16_t size_x = VFile_ReadS16(file);
        VFile_Skip(file, size_z * size_x * 8); // sectors

        VFile_Skip(file, 6); // lighting
        SKIP_ARR_U16(24); // lights
        SKIP_ARR_U16(20); // static meshes
        VFile_Skip(file, 4);
    }

    SKIP_ARR_S32(2); // floor data
    SKIP_ARR_S32(2); // object meshes
    SKIP_ARR_S32(4); // object mesh pointers
    SKIP_ARR_S32(32); // animations
    SKIP_ARR_S32(6); // animation changes
    SKIP_ARR_S32(8); // animation ranges
    SKIP_ARR_S32(2); // animation commands
    SKIP_ARR_S32(4); // animation bones
    SKIP_ARR_S32(2); // animation frames
    SKIP_ARR_S32(18); // objects
    SKIP_ARR_S32(32); // static objects
    SKIP_ARR_S32(20); // object textures
    SKIP_ARR_S32(16); // sprite textures
    SKIP_ARR_S32(8); // sprites sequences
    SKIP_ARR_S32(16); // cameras/sinks
    SKIP_ARR_S32(16); // sound sources

    const int32_t box_count = VFile_ReadS32(file);
    VFile_Skip(file, box_count * 8);
    SKIP_ARR_S32(2); // overlaps
    VFile_Skip(file, box_count * 20); // zones

    SKIP_ARR_S32(2); // animated texture ranges

    Level_ReadItems(file);

#undef SKIP_ARR_S32
#undef SKIP_ARR_U16
}

static M_LAYOUT M_GuessLayout(VFILE *const file)
{
    M_LAYOUT result = LEVEL_LAYOUT_UNKNOWN;
    BENCHMARK benchmark = Benchmark_Start();
    for (M_LAYOUT layout = 0; layout < LEVEL_LAYOUT_NUMBER_OF; layout++) {
        if (M_TryLayout(file, layout)) {
            result = layout;
            break;
        }
    }
    Benchmark_End(&benchmark, nullptr);
    return result;
}

static int32_t M_CompareSampleOffsets(const void *const a, const void *const b)
{
    const SAMPLE_ENTRY *const entry_a = (SAMPLE_ENTRY *)a;
    const SAMPLE_ENTRY *const entry_b = (SAMPLE_ENTRY *)b;
    return entry_a->file_index - entry_b->file_index;
}

static void M_InitialiseSoundEffects(const char *file_name)
{
    BENCHMARK benchmark = Benchmark_Start();
    LEVEL_INFO *info = nullptr;
    SAMPLE_ENTRY *entries = nullptr;

    if (file_name == nullptr) {
        file_name = g_GameFlow.settings.sfx_path;
    }
    const char *full_path =
        File_GetFullPath(file_name == nullptr ? DEFAULT_SFX_PATH : file_name);
    LOG_DEBUG("Loading samples from %s", full_path);

    MYFILE *const fp = File_Open(full_path, FILE_OPEN_READ);
    Memory_FreePointer(&full_path);
    if (fp == nullptr) {
        Shell_ExitSystemFmt("Could not open %s file", file_name);
        goto finish;
    }

    info = Level_GetInfo();
    const int32_t sample_count = info->samples.offset_count;
    entries = Memory_Alloc(sizeof(SAMPLE_ENTRY) * sample_count);
    for (int32_t i = 0; i < sample_count; i++) {
        entries[i].game_index = i;
        entries[i].file_index = info->samples.offsets[i];
    }
    qsort(entries, sample_count, sizeof(SAMPLE_ENTRY), M_CompareSampleOffsets);

    for (int32_t i = 0, current_sample = 0; current_sample < sample_count;
         i++) {
        uint32_t header[11] = {};
        File_ReadData(fp, header, 11 * sizeof(uint32_t));
        if (header[0] != MKTAG('R', 'I', 'F', 'F')
            || header[2] != MKTAG('W', 'A', 'V', 'E')
            || header[9] != MKTAG('d', 'a', 't', 'a')) {
            LOG_ERROR("Unexpected sample header for sample %d", i);
            goto finish;
        }

        const size_t header_size = 11 * sizeof(uint32_t);
        const size_t aligned_size = (header[10] + 1) & ~1;
        const size_t size = aligned_size + header_size;
        const SAMPLE_ENTRY *const entry = &entries[current_sample];
        if (entry->file_index != i) {
            File_Seek(fp, aligned_size, FILE_SEEK_CUR);
            continue;
        }

        char *sample_data = Memory_Alloc(size);
        memcpy(sample_data, header, header_size);
        File_ReadData(fp, sample_data + header_size, aligned_size);
        Sound_LoadSample(entry->game_index, sample_data, size);
        Memory_FreePointer(&sample_data);

        current_sample++;
    }

finish:
    if (fp != nullptr) {
        File_Close(fp);
    }
    Memory_FreePointer(&entries);
    Memory_FreePointer(&info->samples.offsets);
    Benchmark_End(&benchmark, nullptr);
}

static void M_LoadFromFile(const GF_LEVEL *const level)
{
    LOG_DEBUG("%s (num=%d)", level->title, level->num);
    GameBuf_Reset();

    BENCHMARK benchmark = Benchmark_Start();

    const char *full_path = File_GetFullPath(level->path);
    VFILE *const file = VFile_CreateFromPath(full_path);
    Memory_FreePointer(&full_path);

    const M_LAYOUT layout = M_GuessLayout(file);
    if (layout == LEVEL_LAYOUT_UNKNOWN) {
        Shell_ExitSystemFmt("Failed to load %s", level->path);
    }

    VFile_SetPos(file, 4);

    Level_ReadPalettes(file);
    Level_ReadTexturePages(file);
    VFile_Skip(file, 4);
    Level_ReadRooms(file);

    Level_ReadObjectMeshes(file);

    Level_ReadAnims(file);
    Level_ReadAnimChanges(file);
    Level_ReadAnimRanges(file);
    Level_ReadAnimCommands(file);
    Level_ReadAnimBones(file);
    Level_ReadAnimFrames(file);

    Level_ReadObjects(file);
    Level_ReadStaticObjects(file);
    Level_ReadObjectTextures(file);

    Level_ReadSpriteTextures(file);
    Level_ReadSpriteSequences(file);
    Level_ReadCamerasAndSinks(file);
    Level_ReadSoundSources(file);
    Level_ReadPathingData(file);
    Level_ReadAnimatedTextureRanges(file);
    Level_ReadItems(file);

    Level_ReadLightMap(file);
    Level_ReadCinematicFrames(file);
    Level_ReadDemoData(file);
    Level_ReadSamples(file);

    if (layout == LEVEL_LAYOUT_TR2X) {
        VFILE *const embedded_injection = VFile_CreateFromBuffer(
            file->cur_ptr, file->size - VFile_GetPos(file));
        Inject_AppendInjection(embedded_injection);
    }

    VFile_Close(file);
    Benchmark_End(&benchmark, nullptr);
}

static void M_CompleteSetup(const GF_LEVEL *const level)
{
    BENCHMARK benchmark = Benchmark_Start();

    Inject_AllInjections();

    Level_LoadAnimFrames();
    Level_LoadAnimCommands();
    Level_LoadObjectsAndItems();

    Level_LoadTextures();
    Level_LoadTexturePages();
    Level_LoadPalettes();
    Level_LoadFaces();
    Output_DispatchLevelLoad();

    Render_Reset(
        RENDER_RESET_PALETTE | RENDER_RESET_TEXTURES | RENDER_RESET_UVS);

    M_InitialiseSoundEffects(level->settings.sfx_path);

    Benchmark_End(&benchmark, nullptr);
}

bool Level_Load(const GF_LEVEL *const level)
{
    BENCHMARK benchmark = Benchmark_Start();

    Sound_Reset();
    Object_Reset();

    Inject_InitLevel(level);

    M_LoadFromFile(level);
    M_CompleteSetup(level);

    Inject_Cleanup();

    Benchmark_End(&benchmark, nullptr);

    return true;
}

bool Level_Initialise(
    const GF_LEVEL *const level, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    LOG_DEBUG("num=%d type=%d", level->num, level->type);
    if (level->type == GFL_DEMO) {
        Random_SeedDraw(0xD371F947);
        Random_SeedControl(0xD371F947);
    }

    if (level->type != GFL_TITLE && level->type != GFL_DEMO) {
        Gym_SetInventoryOpenEnabled(false);
    }

    if (level->type != GFL_TITLE && level->type != GFL_CUTSCENE) {
        Game_SetCurrentLevel(level);
    }
    GF_SetCurrentLevel(level);
    InitialiseGameFlags();
    g_Lara.item_num = NO_ITEM;
    g_LaraItem = nullptr;

    if (level == nullptr) {
        return false;
    }

    Level_Unload();
    if (!Level_Load(level)) {
        return false;
    }
    GameStringTable_Apply(level);

    Carrier_InitialiseLevel(level);

    if (seq_ctx != GFSC_SAVED) {
        MovableBlock_SetupFloor();
    }

    Effect_InitialiseArray();
    LOT_InitialiseArray();

    Option_Reset();
    Overlay_Reset();
    Overlay_SetHealthBarTimer(100);

    Gym_ResetAssault();
    return true;
}

void Level_Unload(void)
{
    Output_DispatchLevelUnload();
    Camera_Reset();
}

void Level_Init(void)
{
    BENCHMARK benchmark = Benchmark_Start();
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        if (level->type != GFL_NORMAL && level->type != GFL_BONUS) {
            continue;
        }

        VFILE *const file = VFile_CreateFromPath(level->path);
        if (file == nullptr) {
            continue;
        }

        const M_LAYOUT layout = M_GuessLayout(file);
        if (layout != LEVEL_LAYOUT_UNKNOWN) {
            M_SkimLevel(file, layout);
            Stats_CalculateStats();
            const STATS_COMMON stats = {
                .max_secret_count = Stats_GetMaxSecrets(),
                .all_secrets_mask = Stats_GetMaxSecretFlags(),
            };
            Savegame_SetDefaultStats(level, stats);
        }
        GameBuf_Reset();
        VFile_Close(file);
    }

    Benchmark_End(&benchmark, nullptr);
}
