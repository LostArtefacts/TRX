#include "game/level/reader_tr2.h"

#include "game/inject.h"
#include "game/level/reader.h"
#include "game/output/const.h"
#include "game/sound/ids.h"

#define M_SAMPLE_COUNT 370

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
    if (version != 45) {
        return false;
    }

    L_SKIP(1792); // palettes
    L_SKIP_ARR_S32(TEXTURE_PAGE_SIZE * 3); // texture pages
    L_SKIP(4); // unused version number

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
        L_SKIP(size_z * size_x * 8); // sectors

        L_SKIP(6); // lighting
        L_SKIP_ARR_U16(24); // lights
        L_SKIP_ARR_U16(20); // static meshes
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
    L_SKIP_ARR_S32(20); // object textures
    L_SKIP_ARR_S32(16); // sprite textures
    L_SKIP_ARR_S32(8); // sprites sequences
    L_SKIP_ARR_S32(16); // cameras/sinks
    L_SKIP_ARR_S32(16); // sound sources

    int32_t box_count;
    L_TRY_OR_FAIL(VFile_TryReadS32(file, &box_count));
    L_SKIP(box_count * 8);
    L_SKIP_ARR_S32(2); // overlaps
    L_SKIP(box_count * 20); // zones

    L_SKIP_ARR_S32(2); // animated texture ranges
    L_SKIP_ARR_S32(24); // items

    L_SKIP(32 * 256); // light table
    L_SKIP_ARR_U16(16); // cinematic frames
    L_SKIP_ARR_U16(1); // demo data
    L_SKIP(2 * M_SAMPLE_COUNT); // sample lut
    L_SKIP_ARR_S32(8); // sample infos
    L_SKIP_ARR_S32(4); // samples

    if (loader->layout == LEVEL_LAYOUT_TR2X) {
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

    Level_ReadPalettes(loader, file);
    Level_ReadTexturePages(loader, file);
    VFile_Skip(file, 4);
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
    Level_ReadCamerasAndSinks(file);
    Level_ReadSoundSources(file);
    Level_ReadPathingData(loader, file);
    Level_ReadAnimatedTextureRanges(file);
    Level_ReadItems(loader, file);

    Level_ReadLightMap(file);
    Level_ReadCinematicFrames(file);
    Level_ReadDemoData(file);
    Level_ReadSamples(loader, file);

    if (loader->layout == LEVEL_LAYOUT_TR2X) {
        VFILE *const embedded_injection = VFile_CreateFromBuffer(
            file->cur_ptr, file->size - VFile_GetPos(file));
        Inject_AppendInjection(embedded_injection);
    }
    return true;
}

const LEVEL_LOADER g_LevelLoaderTR2 = {
    .game_version = 2,
    .layout = LEVEL_LAYOUT_TR2,
    .load = M_Load,
    .probe = M_Probe,
};

const LEVEL_LOADER g_LevelLoaderTR2X = {
    .game_version = 2,
    .layout = LEVEL_LAYOUT_TR2X,
    .probe = M_Probe,
    .load = M_Load,
};
