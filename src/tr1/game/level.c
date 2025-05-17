#include "game/level.h"

#include "game/effects.h"
#include "game/game.h"
#include "game/inventory_ring/vars.h"
#include "game/lara/common.h"
#include "game/lara/state.h"
#include "game/objects/creatures/mutant.h"
#include "game/objects/creatures/pierre.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/stats.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/benchmark.h>
#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/carrier.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/game/inject.h>
#include <libtrx/game/level.h>
#include <libtrx/game/music.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>
#include <libtrx/virtual_file.h>

#include <stdio.h>
#include <string.h>

typedef enum {
    LEVEL_LAYOUT_UNKNOWN = -1,
    LEVEL_LAYOUT_TR1,
    LEVEL_LAYOUT_TR1_DEMO_PC,
    LEVEL_LAYOUT_NUMBER_OF,
} LEVEL_LAYOUT;

static bool M_TryLayout(VFILE *file, LEVEL_LAYOUT layout);
static LEVEL_LAYOUT M_GuessLayout(VFILE *file);
static void M_LoadFromFile(const GF_LEVEL *level);
static void M_CompleteSetup(const GF_LEVEL *level);
static void M_MarkWaterEdgeVertices(void);

static bool M_TryLayout(VFILE *const file, const LEVEL_LAYOUT layout)
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
    if (version != 32) {
        LOG_ERROR(
            "Unknown level version identifier: %d, expected 32", version, 32);
        return false;
    }

    TRY_OR_FAIL_ARR_S32(TEXTURE_PAGE_SIZE); // textures
    TRY_OR_FAIL(VFile_TrySkip(file, 4));

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
        TRY_OR_FAIL(VFile_TrySkip(file, size_z * size_x * 8));
        TRY_OR_FAIL(VFile_TrySkip(file, 2));

        TRY_OR_FAIL_ARR_U16(18); // lights
        TRY_OR_FAIL_ARR_U16(18); // static meshes
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
    TRY_OR_FAIL_ARR_S32(20); // textures
    TRY_OR_FAIL_ARR_S32(16); // sprites
    TRY_OR_FAIL_ARR_S32(8); // sprites sequences

    if (layout == LEVEL_LAYOUT_TR1_DEMO_PC) {
        TRY_OR_FAIL(VFile_TrySkip(file, 768)); // palette
    }

    TRY_OR_FAIL_ARR_S32(16); // cameras
    TRY_OR_FAIL_ARR_S32(16); // sound effects

    int32_t box_count;
    TRY_OR_FAIL(VFile_TryReadS32(file, &box_count));
    TRY_OR_FAIL(VFile_TrySkip(file, box_count * 20));
    TRY_OR_FAIL_ARR_S32(2); // overlaps
    TRY_OR_FAIL(VFile_TrySkip(file, box_count * 12)); // zones

    TRY_OR_FAIL_ARR_S32(2); // animated texture ranges
    TRY_OR_FAIL_ARR_S32(22); // items

    TRY_OR_FAIL(VFile_TrySkip(file, 32 * 256)); // light table

    if (layout != LEVEL_LAYOUT_TR1_DEMO_PC) {
        TRY_OR_FAIL(VFile_TrySkip(file, 768)); // palette
    }

    TRY_OR_FAIL_ARR_U16(16); // cinematic frames
    TRY_OR_FAIL_ARR_U16(1); // demo data

    TRY_OR_FAIL(VFile_TrySkip(file, 2 * SFX_NUMBER_OF)); // sample lut
    TRY_OR_FAIL_ARR_S32(8); // sample infos
    TRY_OR_FAIL_ARR_S32(1); // sample data
    TRY_OR_FAIL_ARR_S32(4); // samples

#undef TRY_OR_FAIL
#undef TRY_OR_FAIL_ARR_U16
#undef TRY_OR_FAIL_ARR_S32
    return true;
}

static LEVEL_LAYOUT M_GuessLayout(VFILE *const file)
{
    LEVEL_LAYOUT result = LEVEL_LAYOUT_UNKNOWN;
    BENCHMARK benchmark = Benchmark_Start();
    for (LEVEL_LAYOUT layout = 0; layout < LEVEL_LAYOUT_NUMBER_OF; layout++) {
        if (M_TryLayout(file, layout)) {
            result = layout;
            break;
        }
    }
    Benchmark_End(&benchmark, nullptr);
    return result;
}

static void M_LoadFromFile(const GF_LEVEL *const level)
{
    GameBuf_Reset();

    VFILE *file = VFile_CreateFromPath(level->path);
    if (!file) {
        Shell_ExitSystemFmt("Could not open %s", level->path);
    }

    const LEVEL_LAYOUT layout = M_GuessLayout(file);
    if (layout == LEVEL_LAYOUT_UNKNOWN) {
        Shell_ExitSystemFmt("Failed to load %s", level->path);
    }
    VFile_SetPos(file, 4);

    {
        // Read texture pages once the palette is available.
        const int32_t num_pages = VFile_ReadS32(file);
        VFile_Skip(file, num_pages * TEXTURE_PAGE_SIZE * sizeof(uint8_t));
    }

    const int32_t file_level_num = VFile_ReadS32(file);
    LOG_INFO("file level num: %d", file_level_num);

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

    if (layout == LEVEL_LAYOUT_TR1_DEMO_PC) {
        Level_ReadPalettes(file);
    }

    Level_ReadCamerasAndSinks(file);
    Level_ReadSoundSources(file);
    Level_ReadPathingData(file);
    Level_ReadAnimatedTextureRanges(file);
    Level_ReadItems(file);
    Level_ReadLightMap(file);

    if (layout != LEVEL_LAYOUT_TR1_DEMO_PC) {
        Level_ReadPalettes(file);
    }

    Level_ReadCinematicFrames(file);
    Level_ReadDemoData(file);
    Level_ReadSamples(file);

    VFile_SetPos(file, 4);
    Level_ReadTexturePages(file);

    VFile_Close(file);
}

static void M_CompleteSetup(const GF_LEVEL *const level)
{
    BENCHMARK benchmark = Benchmark_Start();

    // We inject explosions sprites and sounds, although in the original game,
    // some levels lack them, resulting in no audio or visual effects when
    // killing mutants. This is to maintain that feature.
    Mutant_ToggleExplosions(Object_Get(O_EXPLOSION_1)->loaded);

    Inject_AllInjections();

    Level_LoadAnimFrames();
    Level_LoadAnimCommands();

    M_MarkWaterEdgeVertices();

    // Must be called post-injection to allow for floor data changes.
    Stats_ObserveRoomsLoad();

    Level_LoadObjectsAndItems();

    Lara_State_Initialise();

    // Configure enemies who carry and drop items
    Carrier_InitialiseLevel(level);

    Level_LoadTextures();
    Level_LoadTexturePages();
    Level_LoadPalettes();
    Level_LoadFaces();
    Output_ObserveLevelLoad();

    // Initialise the sound effects.
    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t sample_count = info->samples.offset_count;
    size_t *sample_sizes = Memory_Alloc(sizeof(size_t) * sample_count);
    const char **sample_pointers = Memory_Alloc(sizeof(char *) * sample_count);
    for (int i = 0; i < sample_count; i++) {
        sample_pointers[i] = info->samples.data + info->samples.offsets[i];
    }

    // NOTE: this assumes that sample pointers are sorted
    for (int32_t i = 0; i < sample_count; i++) {
        const int32_t current_offset = info->samples.offsets[i];
        const int32_t next_offset = i + 1 >= sample_count
            ? info->samples.data_size
            : info->samples.offsets[i + 1];
        sample_sizes[i] = next_offset - current_offset;
    }

    Sound_LoadSamples(sample_count, sample_pointers, sample_sizes);

    Memory_FreePointer(&sample_pointers);
    Memory_FreePointer(&sample_sizes);
    Memory_FreePointer(&info->samples.offsets);

    Benchmark_End(&benchmark, nullptr);
}

static void M_MarkWaterEdgeVertices(void)
{
    if (!g_Config.visuals.fix_texture_issues) {
        return;
    }

    BENCHMARK benchmark = Benchmark_Start();
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        const int32_t y_test =
            (room->flags & RF_UNDERWATER) ? room->max_ceiling : room->min_floor;
        for (int32_t j = 0; j < room->mesh.num_vertices; j++) {
            ROOM_VERTEX *const vertex = &room->mesh.vertices[j];
            if (vertex->pos.y == y_test) {
                vertex->flags |= NO_VERT_MOVE;
            }
        }
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_Load(const GF_LEVEL *const level)
{
    LOG_INFO("%d (%s)", level->num, level->path);
    BENCHMARK benchmark = Benchmark_Start();

    Inject_InitLevel(level);

    M_LoadFromFile(level);
    M_CompleteSetup(level);

    Inject_Cleanup();

    Output_SetSkyboxEnabled(
        g_Config.visuals.enable_skybox && Object_Get(O_SKYBOX)->loaded);

    Benchmark_End(&benchmark, nullptr);
}

void Level_Unload(void)
{
    Output_ObserveLevelUnload();
}

bool Level_Initialise(
    const GF_LEVEL *const level, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    BENCHMARK benchmark = Benchmark_Start();
    LOG_DEBUG("num=%d (%s)", level->num, level->path);
    if (level->type == GFL_DEMO) {
        Random_SeedDraw(0xD371F947);
        Random_SeedControl(0xD371F947);
    }

    g_GameInfo.select_level_num = -1;

    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    if (resume != nullptr) {
        resume->stats.timer = 0;
        resume->stats.secret_flags = 0;
        resume->stats.secret_count = 0;
        resume->stats.pickup_count = 0;
        resume->stats.kill_count = 0;
        resume->stats.ammo_hits = 0;
        resume->stats.ammo_used = 0;
        resume->stats.medipacks_used = 0;
        resume->stats.distance_travelled = 0;
    }

    Game_SetIsLevelComplete(false);
    if (level->type != GFL_TITLE && level->type != GFL_CUTSCENE) {
        Game_SetCurrentLevel((GF_LEVEL *)level);
    }
    GF_SetCurrentLevel((GF_LEVEL *)level);

    Overlay_HideGameInfo();

    Music_ResetTrackFlags();

    Object_Reset();
    Camera_Reset();
    Pierre_Reset();

    Lara_InitialiseLoad(NO_ITEM);
    Level_Unload();
    Level_Load(level);
    GameStringTable_Apply(level);

    if (g_Lara.item_num != NO_ITEM) {
        Lara_Initialise(level);
    }

    if (seq_ctx != GFSC_SAVED) {
        // Avoid initialising the floor before movable block positions have
        // been loaded; this is otherwise handled after savegame loading.
        MovableBlock_SetupFloor();
    }

    Effect_InitialiseArray();
    LOT_InitialiseArray();

    Overlay_Reset();
    Overlay_SetHealthBarTimer(100);

    Music_Stop();
    Music_SetVolume(g_Config.audio.music_volume);
    Sound_ResetEffects();

    const bool disable_music =
        level->type == GFL_TITLE && !g_Config.audio.enable_music_in_menu;
    if (level->music_track >= 0 && !disable_music) {
        Music_Play(
            level->music_track,
            level->type == GFL_CUTSCENE ? MPM_ALWAYS : MPM_LOOPED);
    }

    Viewport_SetFOV(-1);

    Benchmark_End(&benchmark, nullptr);
    return true;
}
