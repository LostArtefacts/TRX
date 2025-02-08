#include "game/level.h"

#include "game/camera.h"
#include "game/carrier.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/inject.h"
#include "game/inventory_ring/vars.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/lara/state.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/creatures/mutant.h"
#include "game/objects/creatures/pierre.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/room.h"
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
#include <libtrx/game/game_buf.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/game/level.h>
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

static LEVEL_INFO m_LevelInfo = {};
static INJECTION_INFO *m_InjectionInfo = nullptr;

static bool M_TryLayout(VFILE *file, LEVEL_LAYOUT layout);
static LEVEL_LAYOUT M_GuessLayout(VFILE *file);
static void M_LoadFromFile(const GF_LEVEL *level);
static void M_LoadObjectMeshes(VFILE *file);
static void M_LoadAnims(VFILE *file);
static void M_LoadAnimChanges(VFILE *file);
static void M_LoadAnimRanges(VFILE *file);
static void M_LoadAnimCommands(VFILE *file);
static void M_LoadAnimBones(VFILE *file);
static void M_LoadAnimFrames(VFILE *file);
static void M_LoadTextures(VFILE *file);
static void M_LoadSprites(VFILE *file);
static void M_LoadBoxes(VFILE *file);
static void M_LoadAnimatedTextures(VFILE *file);
static void M_CompleteSetup(const GF_LEVEL *level);
static void M_MarkWaterEdgeVertices(void);
static size_t M_CalculateMaxVertices(void);

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
    BENCHMARK *const benchmark = Benchmark_Start();
    for (LEVEL_LAYOUT layout = 0; layout < LEVEL_LAYOUT_NUMBER_OF; layout++) {
        if (M_TryLayout(file, layout)) {
            result = layout;
            break;
        }
    }
    Benchmark_End(benchmark, nullptr);
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
    M_LoadObjectMeshes(file);
    M_LoadAnims(file);
    M_LoadAnimChanges(file);
    M_LoadAnimRanges(file);
    M_LoadAnimCommands(file);
    M_LoadAnimBones(file);
    M_LoadAnimFrames(file);
    Level_ReadObjects(file);
    Level_ReadStaticObjects(file);
    M_LoadTextures(file);
    M_LoadSprites(file);
    Level_ReadSpriteSequences(file);

    if (layout == LEVEL_LAYOUT_TR1_DEMO_PC) {
        Level_ReadPalettes(&m_LevelInfo, file);
    }

    Level_ReadCamerasAndSinks(file);
    Level_ReadSoundSources(file);
    M_LoadBoxes(file);
    M_LoadAnimatedTextures(file);
    Level_ReadItems(file);
    Stats_ObserveItemsLoad();
    Level_ReadLightMap(file);

    if (layout != LEVEL_LAYOUT_TR1_DEMO_PC) {
        Level_ReadPalettes(&m_LevelInfo, file);
    }

    Level_ReadCinematicFrames(file);
    Level_ReadDemoData(file);
    Level_ReadSamples(
        &m_LevelInfo, m_InjectionInfo->sfx_count,
        m_InjectionInfo->sfx_data_size, m_InjectionInfo->sample_count, file);

    VFile_SetPos(file, 4);
    Level_ReadTexturePages(
        &m_LevelInfo, m_InjectionInfo->texture_page_count, file);

    VFile_Close(file);
}

static void M_LoadObjectMeshes(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_meshes = VFile_ReadS32(file);
    LOG_INFO("%d object mesh data", num_meshes);

    const size_t data_start_pos = VFile_GetPos(file);
    VFile_Skip(file, num_meshes * sizeof(int16_t));

    m_LevelInfo.mesh_ptr_count = VFile_ReadS32(file);
    LOG_INFO("%d object mesh indices", m_LevelInfo.mesh_ptr_count);
    const int32_t alloc_size = m_LevelInfo.mesh_ptr_count * sizeof(int32_t);
    int32_t *mesh_indices = Memory_Alloc(alloc_size);
    VFile_Read(file, mesh_indices, alloc_size);

    const size_t end_pos = VFile_GetPos(file);
    VFile_SetPos(file, data_start_pos);

    Object_InitialiseMeshes(
        m_LevelInfo.mesh_ptr_count + m_InjectionInfo->mesh_ptr_count);
    Level_ReadObjectMeshes(m_LevelInfo.mesh_ptr_count, mesh_indices, file);

    VFile_SetPos(file, end_pos);
    Memory_FreePointer(&mesh_indices);

    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnims(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anims = VFile_ReadS32(file);
    m_LevelInfo.anims.anim_count = num_anims;
    LOG_INFO("%d anims", num_anims);
    Anim_InitialiseAnims(num_anims + m_InjectionInfo->anim_count);
    Level_ReadAnims(0, num_anims, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimChanges(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_changes = VFile_ReadS32(file);
    m_LevelInfo.anims.change_count = num_anim_changes;
    LOG_INFO("%d anim changes", num_anim_changes);
    Anim_InitialiseChanges(
        num_anim_changes + m_InjectionInfo->anim_change_count);
    Level_ReadAnimChanges(0, num_anim_changes, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimRanges(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_ranges = VFile_ReadS32(file);
    m_LevelInfo.anims.range_count = num_anim_ranges;
    LOG_INFO("%d anim ranges", num_anim_ranges);
    Anim_InitialiseRanges(num_anim_ranges + m_InjectionInfo->anim_range_count);
    Level_ReadAnimRanges(0, num_anim_ranges, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimCommands(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_commands = VFile_ReadS32(file);
    m_LevelInfo.anims.command_count = num_anim_commands;
    LOG_INFO("%d anim commands", num_anim_commands);
    Level_InitialiseAnimCommands(
        num_anim_commands + m_InjectionInfo->anim_cmd_count);
    Level_ReadAnimCommands(0, num_anim_commands, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimBones(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_bones = VFile_ReadS32(file) / ANIM_BONE_SIZE;
    m_LevelInfo.anims.bone_count = num_anim_bones;
    LOG_INFO("%d anim bones", num_anim_bones);
    Anim_InitialiseBones(num_anim_bones + m_InjectionInfo->anim_bone_count);
    Level_ReadAnimBones(0, num_anim_bones, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimFrames(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t raw_data_count = VFile_ReadS32(file);
    m_LevelInfo.anims.frame_count = raw_data_count;
    LOG_INFO("%d raw anim frames", raw_data_count);
    m_LevelInfo.anims.frames = Memory_Alloc(
        sizeof(int16_t)
        * (raw_data_count + m_InjectionInfo->anim_frame_data_count));
    VFile_Read(
        file, m_LevelInfo.anims.frames, sizeof(int16_t) * raw_data_count);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadTextures(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_textures = VFile_ReadS32(file);
    m_LevelInfo.textures.object_count = num_textures;
    LOG_INFO("%d object textures", num_textures);
    Output_InitialiseObjectTextures(
        num_textures + m_InjectionInfo->texture_count);
    Level_ReadObjectTextures(0, 0, num_textures, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadSprites(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_textures = VFile_ReadS32(file);
    m_LevelInfo.textures.sprite_count = num_textures;
    LOG_DEBUG("sprite textures: %d", num_textures);
    Output_InitialiseSpriteTextures(
        num_textures + m_InjectionInfo->sprite_info_count);
    Level_ReadSpriteTextures(0, 0, num_textures, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadBoxes(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    g_NumberBoxes = VFile_ReadS32(file);
    g_Boxes = GameBuf_Alloc(sizeof(BOX_INFO) * g_NumberBoxes, GBUF_BOXES);
    for (int32_t i = 0; i < g_NumberBoxes; i++) {
        BOX_INFO *box = &g_Boxes[i];
        box->left = VFile_ReadS32(file);
        box->right = VFile_ReadS32(file);
        box->top = VFile_ReadS32(file);
        box->bottom = VFile_ReadS32(file);
        box->height = VFile_ReadS16(file);
        box->overlap_index = VFile_ReadS16(file);
    }

    const int32_t num_overlaps = VFile_ReadS32(file);
    g_Overlap = GameBuf_Alloc(sizeof(uint16_t) * num_overlaps, GBUF_OVERLAPS);
    VFile_Read(file, g_Overlap, sizeof(uint16_t) * num_overlaps);

    for (int i = 0; i < 2; i++) {
        g_GroundZone[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_GROUND_ZONE);
        VFile_Read(file, g_GroundZone[i], sizeof(int16_t) * g_NumberBoxes);

        g_GroundZone2[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_GROUND_ZONE);
        VFile_Read(file, g_GroundZone2[i], sizeof(int16_t) * g_NumberBoxes);

        g_FlyZone[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_FLY_ZONE);
        VFile_Read(file, g_FlyZone[i], sizeof(int16_t) * g_NumberBoxes);
    }

    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimatedTextures(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t data_size = VFile_ReadS32(file);
    const size_t end_position =
        VFile_GetPos(file) + data_size * sizeof(int16_t);

    const int16_t num_ranges = VFile_ReadS16(file);
    LOG_INFO("%d animated texture ranges", num_ranges);
    Output_InitialiseAnimatedTextures(num_ranges);
    Level_ReadAnimatedTextureRanges(num_ranges, file);

    VFile_SetPos(file, end_position);
    Benchmark_End(benchmark, nullptr);
}

static void M_CompleteSetup(const GF_LEVEL *const level)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    // We inject explosions sprites and sounds, although in the original game,
    // some levels lack them, resulting in no audio or visual effects when
    // killing mutants. This is to maintain that feature.
    Mutant_ToggleExplosions(Object_Get(O_EXPLOSION_1)->loaded);

    Inject_AllInjections(&m_LevelInfo);

    Level_LoadAnimFrames(&m_LevelInfo);
    Level_LoadAnimCommands();

    M_MarkWaterEdgeVertices();

    // Must be called post-injection to allow for floor data changes.
    Stats_ObserveRoomsLoad();

    Level_LoadObjectsAndItems();

    Lara_State_Initialise();

    // Configure enemies who carry and drop items
    Carrier_InitialiseLevel(level);

    const size_t max_vertices = M_CalculateMaxVertices();
    LOG_INFO("Maximum vertices: %d", max_vertices);
    Output_ReserveVertexBuffer(max_vertices);

    Level_LoadTexturePages(&m_LevelInfo);
    Level_LoadPalettes(&m_LevelInfo);
    Output_DownloadTextures(m_LevelInfo.textures.page_count);

    // Initialise the sound effects.
    const int32_t sample_count = m_LevelInfo.samples.offset_count;
    size_t *sample_sizes = Memory_Alloc(sizeof(size_t) * sample_count);
    const char **sample_pointers = Memory_Alloc(sizeof(char *) * sample_count);
    for (int i = 0; i < sample_count; i++) {
        sample_pointers[i] =
            m_LevelInfo.samples.data + m_LevelInfo.samples.offsets[i];
    }

    // NOTE: this assumes that sample pointers are sorted
    for (int i = 0; i < sample_count; i++) {
        int current_offset = m_LevelInfo.samples.offsets[i];
        int next_offset = i + 1 >= sample_count
            ? m_LevelInfo.samples.data_size
            : m_LevelInfo.samples.offsets[i + 1];
        sample_sizes[i] = next_offset - current_offset;
    }

    Sound_LoadSamples(sample_count, sample_pointers, sample_sizes);

    Memory_FreePointer(&sample_pointers);
    Memory_FreePointer(&sample_sizes);
    Memory_FreePointer(&m_LevelInfo.samples.offsets);

    Benchmark_End(benchmark, nullptr);
}

static void M_MarkWaterEdgeVertices(void)
{
    if (!g_Config.visuals.fix_texture_issues) {
        return;
    }

    BENCHMARK *const benchmark = Benchmark_Start();
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

    Benchmark_End(benchmark, nullptr);
}

static size_t M_CalculateMaxVertices(void)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    int32_t max_vertices = 0;
    for (int32_t i = 0; i < O_NUMBER_OF; i++) {
        const OBJECT *const obj = Object_Get(i);
        if (!obj->loaded) {
            continue;
        }

        for (int32_t j = 0; j < obj->mesh_count; j++) {
            const OBJECT_MESH *const mesh = Object_GetMesh(obj->mesh_idx + j);
            max_vertices = MAX(max_vertices, mesh->num_vertices);
        }
    }

    for (int32_t i = 0; i < MAX_STATIC_OBJECTS; i++) {
        const STATIC_OBJECT_3D *obj = Object_Get3DStatic(i);
        if (!obj->loaded) {
            continue;
        }

        const OBJECT_MESH *const mesh = Object_GetMesh(obj->mesh_idx);
        max_vertices = MAX(max_vertices, mesh->num_vertices);
    }

    for (int32_t i = 0; i < Room_GetCount(); i++) {
        const ROOM *const room = Room_Get(i);
        max_vertices = MAX(max_vertices, room->mesh.num_vertices);
    }

    Benchmark_End(benchmark, nullptr);
    return max_vertices;
}

void Level_Load(const GF_LEVEL *const level)
{
    LOG_INFO("%d (%s)", level->num, level->path);
    BENCHMARK *const benchmark = Benchmark_Start();

    m_InjectionInfo = Memory_Alloc(sizeof(INJECTION_INFO));
    Inject_Init(
        level->injections.count, level->injections.data_paths, m_InjectionInfo);

    M_LoadFromFile(level);
    M_CompleteSetup(level);

    Inject_Cleanup();
    Memory_FreePointer(&m_InjectionInfo);

    Output_SetWaterColor(&level->settings.water_color);
    Output_SetDrawDistFade(level->settings.draw_distance_fade * WALL_L);
    Output_SetDrawDistMax(level->settings.draw_distance_max * WALL_L);
    Output_SetSkyboxEnabled(
        g_Config.visuals.enable_skybox && Object_Get(O_SKYBOX)->loaded);

    Benchmark_End(benchmark, nullptr);
}

bool Level_Initialise(const GF_LEVEL *const level)
{
    BENCHMARK *const benchmark = Benchmark_Start();
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
    }

    g_LevelComplete = false;
    if (level->type != GFL_TITLE && level->type != GFL_CUTSCENE) {
        Game_SetCurrentLevel((GF_LEVEL *)level);
    }
    GF_SetCurrentLevel((GF_LEVEL *)level);
    g_FlipEffect = -1;

    Overlay_HideGameInfo();

    g_FlipStatus = 0;
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        g_FlipMapTable[i] = 0;
    }

    for (int32_t i = 0; i < MAX_CD_TRACKS; i++) {
        g_MusicTrackFlags[i] = 0;
    }

    /* Clear Object Loaded flags */
    for (int32_t i = 0; i < O_NUMBER_OF; i++) {
        Object_Get(i)->loaded = false;
    }
    for (int32_t i = 0; i < MAX_STATIC_OBJECTS; i++) {
        Object_Get2DStatic(i)->loaded = false;
        Object_Get3DStatic(i)->loaded = false;
    }

    Camera_Reset();
    Pierre_Reset();

    Lara_InitialiseLoad(NO_ITEM);
    Level_Load(level);
    GameStringTable_Apply(level);

    if (g_Lara.item_num != NO_ITEM) {
        Lara_Initialise(level);
    }

    Effect_InitialiseArray();
    LOT_InitialiseArray();

    Overlay_Init();
    Overlay_BarSetHealthTimer(100);

    Music_Stop();
    Music_SetVolume(g_Config.audio.music_volume);
    Sound_ResetEffects();

    const bool disable_music =
        level->type == GFL_TITLE && !g_Config.audio.enable_music_in_menu;
    if (level->music_track >= 0 && !disable_music) {
        Music_Play(level->music_track, MPM_LOOPED);
    }

    Viewport_SetFOV(-1);

    g_Camera.underwater = false;
    Benchmark_End(benchmark, nullptr);
    return true;
}
