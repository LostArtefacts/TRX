#include "game/level.h"

#include "game/shell.h"
#include "game/stats.h"

#include <libtrx/benchmark.h>
#include <libtrx/config.h>
#include <libtrx/game/carrier.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/inject.h>
#include <libtrx/game/objects/creatures/mutant.h>
#include <libtrx/game/sound.h>
#include <libtrx/memory.h>

typedef enum {
    LEVEL_LAYOUT_UNKNOWN = -1,
    LEVEL_LAYOUT_TR1X,
    LEVEL_LAYOUT_TR1,
    LEVEL_LAYOUT_TR1_DEMO_PC,
    LEVEL_LAYOUT_NUMBER_OF,
} M_LAYOUT;

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

    if (layout == LEVEL_LAYOUT_TR1X) {
        uint32_t inj_magic;
        TRY_OR_FAIL(VFile_TryReadU32(file, &inj_magic));
        TRY_OR_FAIL((inj_magic == INJECTION_MAGIC));
    }

#undef TRY_OR_FAIL
#undef TRY_OR_FAIL_ARR_U16
#undef TRY_OR_FAIL_ARR_S32
    return true;
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

static void M_InitialiseSoundEffects(void)
{
    BENCHMARK benchmark = Benchmark_Start();
    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t sample_count = info->samples.offset_count;

    // TODO: this assumes that sample pointers are sorted - adopt TR2's approach
    // of sorting by index, verifying WAV headers and using WAV sample size.
    for (int32_t i = 0; i < sample_count; i++) {
        const int32_t current_offset = info->samples.offsets[i];
        const int32_t next_offset = i + 1 >= sample_count
            ? info->samples.data_size
            : info->samples.offsets[i + 1];

        const char *const sample_data = &info->samples.data[current_offset];
        const size_t sample_size = next_offset - current_offset;
        Sound_LoadSampleData(i, sample_data, sample_size);
    }

    Memory_FreePointer(&info->samples.offsets);
    Benchmark_End(&benchmark, nullptr);
}

static void M_LoadFromFile(const GF_LEVEL *const level)
{
    GameBuf_Reset();

    BENCHMARK benchmark = Benchmark_Start();

    VFILE *const file = VFile_CreateFromPath(level->path);
    if (file == nullptr) {
        Shell_ExitSystemFmt("Could not open %s", level->path);
    }

    const M_LAYOUT layout = M_GuessLayout(file);
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

    if (layout == LEVEL_LAYOUT_TR1X) {
        VFILE *const embedded_injection = VFile_CreateFromBuffer(
            file->cur_ptr, file->size - VFile_GetPos(file));
        Inject_AppendInjection(embedded_injection);
    }

    VFile_SetPos(file, 4);
    Level_ReadTexturePages(file);

    VFile_Close(file);
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

static void M_CompleteSetup(const GF_LEVEL *const level)
{
    BENCHMARK benchmark = Benchmark_Start();

#if TR_VERSION == 1
    // We inject explosions sprites and sounds, although in the original game,
    // some levels lack them, resulting in no audio or visual effects when
    // killing mutants. This is to maintain that feature.
    Mutant_ToggleExplosions(Object_Get(O_EXPLOSION_1)->loaded);
#endif

    Inject_AllInjections();

    Level_LoadAnimFrames();
    Level_LoadAnimCommands();
#if TR_VERSION == 1
    M_MarkWaterEdgeVertices();

    // Must be called post-injection to allow for floor data changes.
    Stats_ObserveRoomsLoad();
#else
    Level_LoadWalkables();
#endif
    Level_LoadObjectsAndItems();

    // Configure enemies who carry and drop items
    Carrier_InitialiseLevel(level);

    Level_LoadTextures();
    Level_LoadTexturePages();
    Level_LoadPalettes();
    Level_LoadFaces();

    Output_SetSkyboxEnabled(Object_Get(O_SKYBOX)->loaded);
    Output_DispatchLevelLoad();
#if TR_VERSION == 1
    M_InitialiseSoundEffects();
#else
    M_InitialiseSoundEffects(level->settings.sfx_path);
#endif

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

    Benchmark_End(&benchmark, nullptr);
}
