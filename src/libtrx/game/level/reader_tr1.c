#include "game/level/reader_tr1.h"

#include "benchmark.h"
#include "game/inject.h"
#include "game/level/reader.h"
#include "game/output/const.h"
#include "game/sound/ids.h"
#include "log.h"

#define M_SAMPLE_COUNT 256

static bool M_Probe(const LEVEL_LOADER *const loader, VFILE *const file)
{
    // TODO: clang-format <20 formats this wrongly
    // clang-format off
#define L_TRY_OR_FAIL(call)                                                      \
    if (!call) {                                                               \
        return false;                                                          \
    }
#define L_SKIP(size) L_TRY_OR_FAIL(VFile_TrySkip(file, size));
#define L_SKIP_ARR_S32(size)                                              \
    {                                                                          \
        int32_t num;                                                           \
        L_TRY_OR_FAIL(VFile_TryReadS32(file, &num));                             \
        L_SKIP(num * size);                          \
    }
#define L_SKIP_ARR_U16(size)                                              \
    {                                                                          \
        uint16_t num;                                                          \
        L_TRY_OR_FAIL(VFile_TryReadU16(file, &num));                             \
        L_SKIP(num * size);                          \
    }
    // clang-format on

    VFile_SetPos(file, 0);

    int32_t version;
    L_TRY_OR_FAIL(VFile_TryReadS32(file, &version));
    if (version != 32) {
        return false;
    }

    L_SKIP_ARR_S32(TEXTURE_PAGE_SIZE); // textures
    L_SKIP(4);

    uint16_t room_count;
    L_TRY_OR_FAIL(VFile_TryReadU16(file, &room_count));
    for (int32_t i = 0; i < room_count; i++) {
        L_SKIP(16);
        L_SKIP_ARR_S32(2); // meshes
        L_SKIP_ARR_U16(32); // portals

        int16_t size_z;
        int16_t size_x;
        L_TRY_OR_FAIL(VFile_TryReadS16(file, &size_z));
        L_TRY_OR_FAIL(VFile_TryReadS16(file, &size_x));
        L_SKIP(size_z * size_x * 8);
        L_SKIP(2);

        L_SKIP_ARR_U16(18); // lights
        L_SKIP_ARR_U16(18); // static meshes
        L_SKIP(4);
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
    L_SKIP_ARR_S32(20); // textures
    L_SKIP_ARR_S32(16); // sprites
    L_SKIP_ARR_S32(8); // sprites sequences

    if (loader->layout == LEVEL_LAYOUT_TR1_DEMO_PC) {
        L_SKIP(768); // palette
    }

    L_SKIP_ARR_S32(16); // cameras
    L_SKIP_ARR_S32(16); // sound effects

    int32_t box_count;
    L_TRY_OR_FAIL(VFile_TryReadS32(file, &box_count));
    L_SKIP(box_count * 20);
    L_SKIP_ARR_S32(2); // overlaps
    L_SKIP(box_count * 12); // zones

    L_SKIP_ARR_S32(2); // animated texture ranges
    L_SKIP_ARR_S32(22); // items

    L_SKIP(32 * 256); // light table

    if (loader->layout != LEVEL_LAYOUT_TR1_DEMO_PC) {
        L_SKIP(768); // palette
    }

    L_SKIP_ARR_U16(16); // cinematic frames
    L_SKIP_ARR_U16(1); // demo data

    L_SKIP(2 * M_SAMPLE_COUNT); // sample lut
    L_SKIP_ARR_S32(8); // sample infos
    L_SKIP_ARR_S32(1); // sample data
    L_SKIP_ARR_S32(4); // samples

    if (loader->layout == LEVEL_LAYOUT_TR1X) {
        uint32_t inj_magic;
        L_TRY_OR_FAIL(VFile_TryReadU32(file, &inj_magic));
        L_TRY_OR_FAIL((inj_magic == INJECTION_MAGIC));
    }

#undef L_TRY_OR_FAIL
#undef L_SKIP_ARR_U16
#undef L_SKIP_ARR_S32
    return true;
}

static bool M_Load(const LEVEL_LOADER *const loader, VFILE *const file)
{
    VFile_SetPos(file, 4);

    // Read texture pages once the palette is available.
    const int32_t num_pages = VFile_ReadS32(file);
    VFile_Skip(file, num_pages * TEXTURE_PAGE_SIZE * sizeof(uint8_t));

    const int32_t file_level_num = VFile_ReadS32(file);
    LOG_INFO("file level num: %d", file_level_num);

    Level_ReadRooms(loader, file);
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
    Level_ReadSpriteSequences(loader, file);

    if (loader->layout == LEVEL_LAYOUT_TR1_DEMO_PC) {
        Level_ReadPalettes(loader, file);
    }

    Level_ReadCamerasAndSinks(file);
    Level_ReadSoundSources(file);
    Level_ReadPathingData(loader, file);
    Level_ReadAnimatedTextureRanges(file);
    Level_ReadItems(loader, file);
    Level_ReadLightMap(file);

    if (loader->layout != LEVEL_LAYOUT_TR1_DEMO_PC) {
        Level_ReadPalettes(loader, file);
    }

    Level_ReadCinematicFrames(file);
    Level_ReadDemoData(file);
    Level_ReadSamples(loader, file);

    if (loader->layout == LEVEL_LAYOUT_TR1X) {
        VFILE *const embedded_injection = VFile_CreateFromBuffer(
            file->cur_ptr, file->size - VFile_GetPos(file));
        Inject_AppendInjection(embedded_injection);
    }

    VFile_SetPos(file, 4);
    Level_ReadTexturePages(loader, file);

    return true;
}

const LEVEL_LOADER g_LevelLoaderTR1 = {
    .game_version = 1,
    .layout = LEVEL_LAYOUT_TR1,
    .probe = M_Probe,
    .load = M_Load,
};

const LEVEL_LOADER g_LevelLoaderTR1DemoPC = {
    .game_version = 1,
    .layout = LEVEL_LAYOUT_TR1_DEMO_PC,
    .probe = M_Probe,
    .load = M_Load,
};

const LEVEL_LOADER g_LevelLoaderTR1X = {
    .game_version = 1,
    .layout = LEVEL_LAYOUT_TR1X,
    .probe = M_Probe,
    .load = M_Load,
};
