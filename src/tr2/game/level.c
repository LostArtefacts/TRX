#include "game/level.h"

#include "decomp/decomp.h"
#include "decomp/savegame.h"
#include "game/camera.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/inject.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/render/common.h"
#include "game/room.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/benchmark.h>
#include <libtrx/debug.h>
#include <libtrx/engine/audio.h>
#include <libtrx/filesystem.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/game/level.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/virtual_file.h>

static LEVEL_INFO m_LevelInfo = {};

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
static void M_LoadSoundEffects(VFILE *file);
static void M_LoadBoxes(VFILE *file);
static void M_LoadAnimatedTextures(VFILE *file);
static void M_LoadSamples(VFILE *file);
static void M_CompleteSetup(void);

static void M_LoadObjectMeshes(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_meshes = VFile_ReadS32(file);
    LOG_INFO("object mesh data: %d", num_meshes);

    const size_t data_start_pos = VFile_GetPos(file);
    VFile_Skip(file, num_meshes * sizeof(int16_t));

    const int32_t num_mesh_ptrs = VFile_ReadS32(file);
    LOG_INFO("object mesh indices: %d", num_mesh_ptrs);
    int32_t *const mesh_indices =
        (int32_t *)Memory_Alloc(sizeof(int32_t) * num_mesh_ptrs);
    VFile_Read(file, mesh_indices, sizeof(int32_t) * num_mesh_ptrs);

    const size_t end_pos = VFile_GetPos(file);
    VFile_SetPos(file, data_start_pos);

    Object_InitialiseMeshes(num_mesh_ptrs);
    Level_ReadObjectMeshes(num_mesh_ptrs, mesh_indices, file);

    VFile_SetPos(file, end_pos);
    Memory_Free(mesh_indices);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnims(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anims = VFile_ReadS32(file);
    LOG_INFO("anims: %d", num_anims);
    Anim_InitialiseAnims(num_anims);
    Level_ReadAnims(0, num_anims, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimChanges(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_changes = VFile_ReadS32(file);
    LOG_INFO("anim changes: %d", num_anim_changes);
    Anim_InitialiseChanges(num_anim_changes);
    Level_ReadAnimChanges(0, num_anim_changes, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimRanges(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_ranges = VFile_ReadS32(file);
    LOG_INFO("anim ranges: %d", num_anim_ranges);
    Anim_InitialiseRanges(num_anim_ranges);
    Level_ReadAnimRanges(0, num_anim_ranges, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimCommands(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_commands = VFile_ReadS32(file);
    LOG_INFO("anim commands: %d", num_anim_commands);
    Level_InitialiseAnimCommands(num_anim_commands);
    Level_ReadAnimCommands(0, num_anim_commands, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimBones(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_anim_bones = VFile_ReadS32(file) / ANIM_BONE_SIZE;
    LOG_INFO("anim bones: %d", num_anim_bones);
    Anim_InitialiseBones(num_anim_bones);
    Level_ReadAnimBones(0, num_anim_bones, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimFrames(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t raw_data_count = VFile_ReadS32(file);
    m_LevelInfo.anims.frame_count = raw_data_count;
    LOG_INFO("anim frame data size: %d", raw_data_count);
    m_LevelInfo.anims.frames = Memory_Alloc(sizeof(int16_t) * raw_data_count);
    VFile_Read(
        file, m_LevelInfo.anims.frames, sizeof(int16_t) * raw_data_count);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadTextures(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_textures = VFile_ReadS32(file);
    LOG_INFO("object textures: %d", num_textures);
    Output_InitialiseObjectTextures(num_textures);
    Level_ReadObjectTextures(0, 0, num_textures, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadSprites(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const int32_t num_textures = VFile_ReadS32(file);
    LOG_DEBUG("sprite textures: %d", num_textures);
    Output_InitialiseSpriteTextures(num_textures);
    Level_ReadSpriteTextures(0, 0, num_textures, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadBoxes(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    g_BoxCount = VFile_ReadS32(file);
    g_Boxes = GameBuf_Alloc(sizeof(BOX_INFO) * g_BoxCount, GBUF_BOXES);
    for (int32_t i = 0; i < g_BoxCount; i++) {
        BOX_INFO *const box = &g_Boxes[i];
        box->left = VFile_ReadU8(file);
        box->right = VFile_ReadU8(file);
        box->top = VFile_ReadU8(file);
        box->bottom = VFile_ReadU8(file);
        box->height = VFile_ReadS16(file);
        box->overlap_index = VFile_ReadS16(file);
    }

    const int32_t num_overlaps = VFile_ReadS32(file);
    g_Overlap = GameBuf_Alloc(sizeof(uint16_t) * num_overlaps, GBUF_OVERLAPS);
    VFile_Read(file, g_Overlap, sizeof(uint16_t) * num_overlaps);

    for (int32_t i = 0; i < 2; i++) {
        for (int32_t j = 0; j < 4; j++) {
            const bool skip = j == 2
                || (j == 1 && !Object_Get(O_SPIDER)->loaded
                    && !Object_Get(O_SKIDOO_ARMED)->loaded)
                || (j == 3 && !Object_Get(O_YETI)->loaded
                    && !Object_Get(O_WORKER_3)->loaded);

            if (skip) {
                VFile_Skip(file, sizeof(int16_t) * g_BoxCount);
                continue;
            }

            g_GroundZone[j][i] =
                GameBuf_Alloc(sizeof(int16_t) * g_BoxCount, GBUF_GROUND_ZONE);
            VFile_Read(file, g_GroundZone[j][i], sizeof(int16_t) * g_BoxCount);
        }

        g_FlyZone[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_BoxCount, GBUF_FLY_ZONE);
        VFile_Read(file, g_FlyZone[i], sizeof(int16_t) * g_BoxCount);
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

static void M_LoadSamples(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    int32_t *sample_offsets = nullptr;

    Audio_Sample_CloseAll();
    Audio_Sample_UnloadAll();

    int16_t *const sample_lut = Sound_GetSampleLUT();
    VFile_Read(file, sample_lut, sizeof(int16_t) * SFX_NUMBER_OF);
    const int32_t num_sample_infos = VFile_ReadS32(file);
    LOG_DEBUG("sample infos: %d", num_sample_infos);
    if (num_sample_infos == 0) {
        goto finish;
    }

    Sound_InitialiseSampleInfos(num_sample_infos);
    for (int32_t i = 0; i < num_sample_infos; i++) {
        SAMPLE_INFO *const sample_info = Sound_GetSampleInfoByIdx(i);
        sample_info->number = VFile_ReadS16(file);
        sample_info->volume = VFile_ReadS16(file);
        sample_info->randomness = VFile_ReadS16(file);
        sample_info->flags = VFile_ReadS16(file);
    }

    const int32_t num_samples = VFile_ReadS32(file);
    LOG_DEBUG("samples: %d", num_samples);
    if (!num_samples) {
        goto finish;
    }

    sample_offsets = Memory_Alloc(sizeof(int32_t) * num_samples);
    VFile_Read(file, sample_offsets, sizeof(int32_t) * num_samples);

    const char *const file_name = "data\\main.sfx";
    const char *full_path = File_GetFullPath(file_name);
    LOG_DEBUG("Loading samples from %s", full_path);
    MYFILE *const fp = File_Open(full_path, FILE_OPEN_READ);
    Memory_FreePointer(&full_path);

    if (fp == nullptr) {
        Shell_ExitSystemFmt("Could not open %s file", file_name);
        goto finish;
    }

    // TODO: refactor these WAVE/RIFF shenanigans
    int32_t sample_id = 0;
    for (int32_t i = 0; sample_id < num_samples; i++) {
        char header[0x2C];
        File_ReadData(fp, header, 0x2C);
        if (*(int32_t *)(header + 0) != 0x46464952
            || *(int32_t *)(header + 8) != 0x45564157
            || *(int32_t *)(header + 36) != 0x61746164) {
            LOG_ERROR("Unexpected sample header for sample %d", i);
            goto finish;
        }
        const int32_t data_size = *(int32_t *)(header + 0x28);
        const int32_t aligned_size = (data_size + 1) & ~1;

        if (sample_offsets[sample_id] != i) {
            File_Seek(fp, aligned_size, FILE_SEEK_CUR);
            continue;
        }

        const size_t sample_data_size = 0x2C + aligned_size;
        char *sample_data = Memory_Alloc(sample_data_size);
        memcpy(sample_data, header, 0x2C);
        File_ReadData(fp, sample_data + 0x2C, aligned_size);

        const bool result =
            Audio_Sample_LoadSingle(sample_id, sample_data, sample_data_size);
        Memory_FreePointer(&sample_data);

        if (!result) {
            goto finish;
        }

        sample_id++;
    }
    File_Close(fp);

finish:
    Memory_FreePointer(&sample_offsets);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadFromFile(const GF_LEVEL *const level)
{
    LOG_DEBUG("%s (num=%d)", level->title, level->num);
    GameBuf_Reset();

    BENCHMARK *const benchmark = Benchmark_Start();

    const char *full_path = File_GetFullPath(level->path);
    strcpy(g_LevelFileName, full_path);
    VFILE *const file = VFile_CreateFromPath(full_path);
    Memory_FreePointer(&full_path);

    const int32_t version = VFile_ReadS32(file);
    if (version != 45) {
        Shell_ExitSystemFmt(
            "Unexpected level version (%d, expected: %d, path: %s)", level->num,
            45, level->path);
    }

    Level_ReadPalettes(&m_LevelInfo, file);
    Level_ReadTexturePages(&m_LevelInfo, 0, file);
    VFile_Skip(file, 4);
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
    Level_ReadCamerasAndSinks(file);
    Level_ReadSoundSources(file);
    M_LoadBoxes(file);
    M_LoadAnimatedTextures(file);
    Level_ReadItems(file);

    Level_ReadLightMap(file);
    Level_ReadCinematicFrames(file);
    Level_ReadDemoData(file);
    M_LoadSamples(file);

    VFile_Close(file);
    Benchmark_End(benchmark, nullptr);
}

static void M_CompleteSetup(void)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    Inject_AllInjections();

    Level_LoadAnimFrames(&m_LevelInfo);
    Level_LoadAnimCommands();
    Level_LoadObjectsAndItems();

    Level_LoadTexturePages(&m_LevelInfo);
    Level_LoadPalettes(&m_LevelInfo);
    Output_InitialiseNamedColors();

    Render_Reset(
        RENDER_RESET_PALETTE | RENDER_RESET_TEXTURES | RENDER_RESET_UVS);

    Benchmark_End(benchmark, nullptr);
}

bool Level_Load(const GF_LEVEL *const level)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    for (int32_t i = 0; i < O_NUMBER_OF; i++) {
        Object_Get(i)->loaded = false;
    }
    for (int32_t i = 0; i < MAX_STATIC_OBJECTS; i++) {
        Object_Get2DStatic(i)->loaded = false;
        Object_Get3DStatic(i)->loaded = false;
    }

    Inject_Init(level->injections.count, level->injections.data_paths);

    M_LoadFromFile(level);
    M_CompleteSetup();

    Inject_Cleanup();

    Benchmark_End(benchmark, nullptr);

    return true;
}

bool Level_Initialise(const GF_LEVEL *const level)
{
    LOG_DEBUG("num=%d type=%d", level->num, level->type);
    if (level->type == GFL_DEMO) {
        Random_SeedDraw(0xD371F947);
        Random_SeedControl(0xD371F947);
    }

    if (level->type != GFL_TITLE && level->type != GFL_DEMO) {
        g_GymInvOpenEnabled = false;
    }

    if (level->type != GFL_TITLE && level->type != GFL_CUTSCENE) {
        Game_SetCurrentLevel(level);
    }
    GF_SetCurrentLevel(level);
    InitialiseGameFlags();
    g_Lara.item_num = NO_ITEM;

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

    g_IsAssaultTimerActive = false;
    g_IsAssaultTimerDisplay = false;
    g_Camera.underwater = 0;
    return true;
}

void Level_Unload(void)
{
    strcpy(g_LevelFileName, "");
    Output_InitialiseTexturePages(0, true);
    Output_InitialiseObjectTextures(0);

    if (Output_GetBackgroundType() == BK_OBJECT) {
        Output_UnloadBackground();
    }
}
