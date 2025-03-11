#include "game/level.h"

#include "decomp/decomp.h"
#include "decomp/savegame.h"
#include "game/camera.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
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
#include <libtrx/game/inject.h>
#include <libtrx/game/level.h>
#include <libtrx/game/objects/traps/movable_block.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/virtual_file.h>

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
static void M_InitialiseSoundEffects(void);
static void M_CompleteSetup(void);

static void M_LoadObjectMeshes(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t num_meshes = VFile_ReadS32(file);
    LOG_INFO("object mesh data: %d", num_meshes);

    const size_t data_start_pos = VFile_GetPos(file);
    VFile_Skip(file, num_meshes * sizeof(int16_t));

    info->mesh_ptr_count = VFile_ReadS32(file);
    LOG_INFO("object mesh indices: %d", info->mesh_ptr_count);
    const int32_t alloc_size = info->mesh_ptr_count * sizeof(int32_t);
    int32_t *mesh_indices = Memory_Alloc(alloc_size);
    VFile_Read(file, mesh_indices, alloc_size);

    const size_t end_pos = VFile_GetPos(file);
    VFile_SetPos(file, data_start_pos);

    Object_InitialiseMeshes(
        info->mesh_ptr_count + Inject_GetDataCount(IDT_MESH_POINTERS));
    Level_ReadObjectMeshes(info->mesh_ptr_count, mesh_indices, file);

    VFile_SetPos(file, end_pos);
    Memory_FreePointer(&mesh_indices);

    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnims(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t num_anims = VFile_ReadS32(file);
    info->anims.anim_count = num_anims;
    LOG_INFO("anims: %d", num_anims);
    Anim_InitialiseAnims(num_anims + Inject_GetDataCount(IDT_ANIMS));
    Level_ReadAnims(0, num_anims, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimChanges(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t num_anim_changes = VFile_ReadS32(file);
    info->anims.change_count = num_anim_changes;
    LOG_INFO("anim changes: %d", num_anim_changes);
    Anim_InitialiseChanges(
        num_anim_changes + Inject_GetDataCount(IDT_ANIM_CHANGES));
    Level_ReadAnimChanges(0, num_anim_changes, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimRanges(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t num_anim_ranges = VFile_ReadS32(file);
    info->anims.range_count = num_anim_ranges;
    LOG_INFO("anim ranges: %d", num_anim_ranges);
    Anim_InitialiseRanges(
        num_anim_ranges + Inject_GetDataCount(IDT_ANIM_RANGES));
    Level_ReadAnimRanges(0, num_anim_ranges, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimCommands(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t num_anim_commands = VFile_ReadS32(file);
    info->anims.command_count = num_anim_commands;
    LOG_INFO("anim commands: %d", num_anim_commands);
    info->anims.commands = Memory_Alloc(
        sizeof(int16_t)
        * (num_anim_commands + Inject_GetDataCount(IDT_ANIM_COMMANDS)));
    VFile_Read(file, info->anims.commands, sizeof(int16_t) * num_anim_commands);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimBones(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t num_anim_bones = VFile_ReadS32(file) / ANIM_BONE_SIZE;
    info->anims.bone_count = num_anim_bones;
    LOG_INFO("anim bones: %d", num_anim_bones);
    Anim_InitialiseBones(num_anim_bones + Inject_GetDataCount(IDT_ANIM_BONES));
    Level_ReadAnimBones(0, num_anim_bones, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadAnimFrames(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t raw_data_count = VFile_ReadS32(file);
    info->anims.frame_count = raw_data_count;
    LOG_INFO("raw anim frames: %d", raw_data_count);
    info->anims.frames = Memory_Alloc(
        sizeof(int16_t)
        * (raw_data_count + Inject_GetDataCount(IDT_ANIM_FRAMES)));
    VFile_Read(file, info->anims.frames, sizeof(int16_t) * raw_data_count);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadTextures(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t num_textures = VFile_ReadS32(file);
    info->textures.object_count = num_textures;
    LOG_INFO("object textures: %d", num_textures);
    Output_InitialiseObjectTextures(
        num_textures + Inject_GetDataCount(IDT_OBJECT_TEXTURES));
    Level_ReadObjectTextures(0, 0, num_textures, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_LoadSprites(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LEVEL_INFO *const info = Level_GetInfo();
    const int32_t num_textures = VFile_ReadS32(file);
    info->textures.sprite_count = num_textures;
    LOG_DEBUG("sprite textures: %d", num_textures);
    Output_InitialiseSpriteTextures(
        num_textures + Inject_GetDataCount(IDT_SPRITE_TEXTURES));
    Level_ReadSpriteTextures(0, 0, num_textures, file);
    Benchmark_End(benchmark, nullptr);
}

static void M_InitialiseSoundEffects(void)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    const char *const file_name = "data\\main.sfx";
    const char *full_path = File_GetFullPath(file_name);
    LOG_DEBUG("Loading samples from %s", full_path);
    MYFILE *const fp = File_Open(full_path, FILE_OPEN_READ);
    Memory_FreePointer(&full_path);

    LEVEL_INFO *const info = Level_GetInfo();
    if (fp == nullptr) {
        Shell_ExitSystemFmt("Could not open %s file", file_name);
        goto finish;
    }

    // TODO: refactor these WAVE/RIFF shenanigans
    int32_t sample_id = 0;
    for (int32_t i = 0; sample_id < info->samples.offset_count; i++) {
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

        if (info->samples.offsets[sample_id] != i) {
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

finish:
    if (fp != nullptr) {
        File_Close(fp);
    }
    Memory_FreePointer(&info->samples.offsets);
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

    Level_ReadPalettes(file);
    Level_ReadTexturePages(file);
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
    Level_ReadPathingData(file);
    Level_ReadAnimatedTextureRanges(file);
    Level_ReadItems(file);

    Level_ReadLightMap(file);
    Level_ReadCinematicFrames(file);
    Level_ReadDemoData(file);
    Level_ReadSamples(file);

    VFile_Close(file);
    Benchmark_End(benchmark, nullptr);
}

static void M_CompleteSetup(void)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    Inject_AllInjections();

    Level_LoadAnimFrames();
    Level_LoadAnimCommands();
    Level_LoadObjectsAndItems();

    Level_LoadTexturePages();
    Level_LoadPalettes();
    Output_InitialiseNamedColors();

    Render_Reset(
        RENDER_RESET_PALETTE | RENDER_RESET_TEXTURES | RENDER_RESET_UVS);

    M_InitialiseSoundEffects();

    Benchmark_End(benchmark, nullptr);
}

bool Level_Load(const GF_LEVEL *const level)
{
    BENCHMARK *const benchmark = Benchmark_Start();

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
    M_CompleteSetup();

    Inject_Cleanup();

    Benchmark_End(benchmark, nullptr);

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
        g_GymInvOpenEnabled = false;
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

    Camera_Reset();
}
