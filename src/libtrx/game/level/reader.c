#include "game/level/reader.h"

#include "benchmark.h"
#include "config.h"
#include "debug.h"
#include "filesystem.h"
#include "game/carrier.h"
#include "game/game_buf.h"
#include "game/inject.h"
#include "game/level.h"
#include "game/level/reader_tr1.h"
#include "game/level/reader_tr2.h"
#include "game/objects.h"
#include "game/objects/creatures/mutant.h"
#include "game/output.h"
#include "game/rooms.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/stats.h"
#include "log.h"
#include "memory.h"
#include "version.h"

#define M_DEFAULT_SFX_PATH "data/main.sfx"

#if TR_VERSION == 1
// TODO: refactor me
extern void Stats_ObserveRoomsLoad(void);
#endif

typedef struct {
    int32_t game_index;
    int32_t file_index;
} M_SAMPLE_ENTRY;

const LEVEL_LOADER *g_LevelLoaders[] = {
#if DEBUG || TR_VERSION == 1
    &g_LevelLoaderTR1X, &g_LevelLoaderTR1, &g_LevelLoaderTR1DemoPC,
#endif
#if DEBUG || TR_VERSION == 2
    &g_LevelLoaderTR2X, &g_LevelLoaderTR2,
#endif
};

static int32_t M_CompareSampleOffsets(const void *const a, const void *const b)
{
    const M_SAMPLE_ENTRY *const entry_a = (M_SAMPLE_ENTRY *)a;
    const M_SAMPLE_ENTRY *const entry_b = (M_SAMPLE_ENTRY *)b;
    return entry_a->file_index - entry_b->file_index;
}

static void M_InitialiseSamplesFromFile(const char *file_name)
{
    BENCHMARK benchmark = Benchmark_Start();
    M_SAMPLE_ENTRY *entries = nullptr;
    LEVEL_INFO *const info = Level_GetInfo();

    if (file_name == nullptr) {
        file_name = g_GameFlow.settings.sfx_path;
    }
    if (file_name == nullptr) {
        file_name = M_DEFAULT_SFX_PATH;
    }
    const char *full_path = File_GetFullPath(file_name);
    LOG_DEBUG("Loading samples from %s", full_path);

    MYFILE *const fp = File_Open(full_path, FILE_OPEN_READ);
    Memory_FreePointer(&full_path);
    if (fp == nullptr) {
        LOG_ERROR("Could not open %s samples file", file_name);
        goto finish;
    }

    const int32_t sample_count = info->samples.offset_count;
    entries = Memory_Alloc(sizeof(M_SAMPLE_ENTRY) * sample_count);
    for (int32_t i = 0; i < sample_count; i++) {
        entries[i].game_index = i;
        entries[i].file_index = info->samples.offsets[i];
    }
    qsort(
        entries, sample_count, sizeof(M_SAMPLE_ENTRY), M_CompareSampleOffsets);

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
        const M_SAMPLE_ENTRY *const entry = &entries[current_sample];
        if (entry->file_index != i) {
            File_Seek(fp, aligned_size, FILE_SEEK_CUR);
            continue;
        }

        char *sample_data = Memory_Alloc(size);
        memcpy(sample_data, header, header_size);
        File_ReadData(fp, sample_data + header_size, aligned_size);
        Sound_LoadSampleData(entry->game_index, sample_data, size);
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

static void M_InitialiseSamplesFromLevelInfo(void)
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

const LEVEL_LOADER *Level_GuessLoader(VFILE *const file)
{
    const LEVEL_LOADER *result = nullptr;
    BENCHMARK benchmark = Benchmark_Start();
    for (int32_t i = 0; g_LevelLoaders[i] != nullptr; i++) {
        const LEVEL_LOADER *const loader = g_LevelLoaders[i];
        if (loader->probe(loader, file)) {
            result = loader;
            break;
        }
    }
    Benchmark_End(&benchmark, nullptr);
    return result;
}

LEVEL_LAYOUT Level_GuessLayout(VFILE *const file)
{
    const LEVEL_LOADER *const loader = Level_GuessLoader(file);
    if (loader != nullptr) {
        return loader->layout;
    }
    return LEVEL_LAYOUT_UNKNOWN;
}

static const LEVEL_LOADER *M_LoadFromFile(const GF_LEVEL *const level)
{
    GameBuf_Reset();

    BENCHMARK benchmark = Benchmark_Start();
    VFILE *const file = VFile_CreateFromPath(level->path);
    if (file == nullptr) {
        Shell_ExitSystemFmt("Could not open %s", level->path);
    }

    const LEVEL_LOADER *const loader = Level_GuessLoader(file);
    if (loader == nullptr) {
        Shell_ExitSystemFmt("Failed to load %s", level->path);
    }
    g_TRVersion = loader->game_version;
    ASSERT(loader->load != nullptr);
    loader->load(loader, file);

    VFile_Close(file);
    Benchmark_End(&benchmark, nullptr);
    return loader;
}

static void M_CompleteSetup(
    const LEVEL_LOADER *const loader, const GF_LEVEL *const level)
{
    BENCHMARK benchmark = Benchmark_Start();

#if TR_VERSION == 1
    // We inject explosions sprites and sounds, although in the original game,
    // some levels lack them, resulting in no audio or visual effects when
    // killing mutants. This is to maintain that feature.
    Mutant_ToggleExplosions(Object_Get(O_EXPLOSION_1)->loaded);
#endif

    Inject_AllInjections();

    Level_LoadAnimFrames(loader);
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
    Level_LoadTexturePages(loader);
    Level_LoadPalettes();
    Level_LoadFaces();

    Output_SetSkyboxEnabled(Object_Get(O_SKYBOX)->loaded);
    Output_DispatchLevelLoad();
    if (loader->game_version == 1) {
        M_InitialiseSamplesFromLevelInfo();
    } else {
        M_InitialiseSamplesFromFile(level->settings.sfx_path);
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_Load(const GF_LEVEL *const level)
{
    LOG_INFO("%d (%s)", level->num, level->path);
    BENCHMARK benchmark = Benchmark_Start();

    Inject_InitLevel(level);
    const LEVEL_LOADER *const loader = M_LoadFromFile(level);
    M_CompleteSetup(loader, level);
    Inject_Cleanup();

    Benchmark_End(&benchmark, nullptr);
}
