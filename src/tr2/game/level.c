#include "game/level.h"

#include "game/savegame.h"
#include "game/shell.h"
#include "game/stats.h"

#include <libtrx/benchmark.h>
#include <libtrx/debug.h>
#include <libtrx/game/carrier.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/inject.h>
#include <libtrx/game/sound.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/version.h>

typedef struct {
    int32_t game_index;
    int32_t file_index;
} SAMPLE_ENTRY;

static void M_SkimLevel(const LEVEL_LOADER *const loader, VFILE *const file)
{
    // TODO: clang-format <20 formats this wrongly
    // clang-format off
#define L_SKIP_ARR_S32(size)                                                     \
    {                                                                          \
        const int32_t num = VFile_ReadS32(file);                               \
        VFile_Skip(file, num * size);                                          \
    }
#define L_SKIP_ARR_U16(size)                                                     \
    {                                                                          \
        const uint16_t num = VFile_ReadU16(file);                              \
        VFile_Skip(file, num * size);                                          \
    }
    // clang-format on

    VFile_SetPos(file, 4); // start after version number
    VFile_Skip(file, 1792); // palettes
    L_SKIP_ARR_S32(TEXTURE_PAGE_SIZE * 3); // texture pages
    VFile_Skip(file, 4); // unused version number

    const uint16_t room_count = VFile_ReadU16(file);
    for (int32_t i = 0; i < room_count; i++) {
        VFile_Skip(file, 16);
        L_SKIP_ARR_S32(2); // meshes
        L_SKIP_ARR_U16(32); // portals

        const int16_t size_z = VFile_ReadS16(file);
        const int16_t size_x = VFile_ReadS16(file);
        VFile_Skip(file, size_z * size_x * 8); // sectors

        VFile_Skip(file, 6); // lighting
        L_SKIP_ARR_U16(24); // lights
        L_SKIP_ARR_U16(20); // static meshes
        VFile_Skip(file, 4);
    }

    L_SKIP_ARR_S32(2); // floor data
    L_SKIP_ARR_S32(2); // object meshes
    L_SKIP_ARR_S32(4); // object mesh pointers
    L_SKIP_ARR_S32(32); // animations
    L_SKIP_ARR_S32(6); // animation changes
    L_SKIP_ARR_S32(8); // animation ranges
    L_SKIP_ARR_S32(2); // animation commands
    L_SKIP_ARR_S32(4); // animation bones
    L_SKIP_ARR_S32(2); // animation frames
    L_SKIP_ARR_S32(18); // objects
    L_SKIP_ARR_S32(32); // static objects
    L_SKIP_ARR_S32(20); // object textures
    L_SKIP_ARR_S32(16); // sprite textures
    L_SKIP_ARR_S32(8); // sprites sequences
    L_SKIP_ARR_S32(16); // cameras/sinks
    L_SKIP_ARR_S32(16); // sound sources

    const int32_t box_count = VFile_ReadS32(file);
    VFile_Skip(file, box_count * 8);
    L_SKIP_ARR_S32(2); // overlaps
    VFile_Skip(file, box_count * 20); // zones

    L_SKIP_ARR_S32(2); // animated texture ranges

    Level_ReadItems(loader, file);

#undef L_SKIP_ARR_S32
#undef L_SKIP_ARR_U16
}

// TODO: refactor me
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

        const LEVEL_LOADER *const loader = Level_GuessLoader(file);
        if (loader != nullptr) {
            M_SkimLevel(loader, file);
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
