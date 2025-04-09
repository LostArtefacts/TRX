#include "game/level.h"

#include "decomp/decomp.h"
#include "game/camera.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/gym.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/render/common.h"
#include "game/room.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/stats.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/benchmark.h>
#include <libtrx/debug.h>
#include <libtrx/engine/audio.h>
#include <libtrx/filesystem.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/game/inject.h>
#include <libtrx/game/level.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>
#include <libtrx/virtual_file.h>

#define DEFAULT_SFX_PATH "data/main.sfx"

typedef struct {
    int32_t game_index;
    int32_t file_index;
} SAMPLE_ENTRY;

static int32_t M_CompareSampleOffsets(const void *a, const void *b);
static void M_LoadFromFile(const GF_LEVEL *level);
static void M_InitialiseSoundEffects(const char *const file_name);
static void M_CompleteSetup(const GF_LEVEL *level);
static bool M_SkimLevel(VFILE *file);

static bool M_SkimLevel(VFILE *const file)
{
#define TRY_OR_FAIL(call)                                                      \
    if (!call) {                                                               \
        return false;                                                          \
    }
#define TRY_OR_FAIL_ARR_S32(size)                                              \
    {                                                                          \
        int32_t num;                                                           \
        TRY_OR_FAIL(VFile_TryReadS32(file, &num));                             \
        TRY_OR_FAIL(VFile_TrySkip(file, num *size));                           \
    }
#define TRY_OR_FAIL_ARR_U16(size)                                              \
    {                                                                          \
        uint16_t num;                                                          \
        TRY_OR_FAIL(VFile_TryReadU16(file, &num));                             \
        TRY_OR_FAIL(VFile_TrySkip(file, num *size));                           \
    }

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
    Level_ReadItems(file);

#undef TRY_OR_FAIL
#undef TRY_OR_FAIL_ARR_U16
#undef TRY_OR_FAIL_ARR_S32

    return true;
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

    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t sample_count = info->samples.offset_count;
    entries = Memory_Alloc(sizeof(SAMPLE_ENTRY) * sample_count);
    for (int32_t i = 0; i < sample_count; i++) {
        entries[i].game_index = i;
        entries[i].file_index = info->samples.offsets[i];
    }
    qsort(entries, sample_count, sizeof(SAMPLE_ENTRY), M_CompareSampleOffsets);

    for (int32_t i = 0, current_sample = 0; current_sample < sample_count;
         i++) {
        uint32_t header[11];
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
        const bool result =
            Audio_Sample_LoadSingle(entry->game_index, sample_data, size);
        Memory_FreePointer(&sample_data);

        if (!result) {
            LOG_WARNING("Failed to load sample %d", entry->game_index);
        }

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

    const int32_t version = VFile_ReadS32(file);
    if (version != 45) {
        Shell_ExitSystemFmt(
            "Unexpected level version (%d, expected: %d, path: %s)", level->num,
            45, level->path);
    }

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
    Output_InitialiseNamedColors();

    Render_Reset(
        RENDER_RESET_PALETTE | RENDER_RESET_TEXTURES | RENDER_RESET_UVS);

    M_InitialiseSoundEffects(level->settings.sfx_path);

    Benchmark_End(&benchmark, nullptr);
}

bool Level_Load(const GF_LEVEL *const level)
{
    BENCHMARK benchmark = Benchmark_Start();

    Audio_Sample_CloseAll();
    Audio_Sample_UnloadAll();

    for (int32_t i = 0; i < O_NUMBER_OF; i++) {
        Object_Get(i)->loaded = false;
    }
    for (int32_t i = 0; i < MAX_STATIC_OBJECTS; i++) {
        Object_Get2DStatic(i)->loaded = false;
        Object_Get3DStatic(i)->loaded = false;
    }

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

    if (g_Lara.item_num != NO_ITEM) {
        Lara_Initialise(level);
    }
    GetCarriedItems();

    if (seq_ctx != GFSC_SAVED) {
        MovableBlock_SetupFloor();
    }

    Effect_InitialiseArray();
    LOT_InitialiseArray();
    Overlay_Reset();
    g_HealthBarTimer = 100;
    Sound_StopAll();

    if (Object_Get(O_FINAL_LEVEL_COUNTER)->loaded) {
        InitialiseFinalLevel();
    }

    if (level->music_track != MX_INACTIVE) {
        Music_Play(
            level->music_track,
            level->type == GFL_CUTSCENE ? MPM_ALWAYS : MPM_LOOPED);
    }

    Gym_ResetAssault();
    g_Camera.underwater = 0;
    return true;
}

void Level_Unload(void)
{
    Output_InitialiseTexturePages(0, true);
    Output_InitialiseObjectTextures(0);

    if (Output_GetBackgroundType() == BK_OBJECT) {
        Output_UnloadBackground();
    }

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

        const bool result = M_SkimLevel(file);
        VFile_Close(file);

        if (result) {
            Stats_CalculateStats();
            STATS_COMMON stats = {};
            stats.max_secret_count = Stats_GetSecrets();
            Savegame_SetDefaultStats(level, stats);
        }
        GameBuf_Reset();
    }

    Benchmark_End(&benchmark, nullptr);
}
