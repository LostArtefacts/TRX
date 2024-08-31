#include "game/level.h"

#include "config.h"
#include "game/camera.h"
#include "game/carrier.h"
#include "game/effects.h"
#include "game/gamebuf.h"
#include "game/gameflow.h"
#include "game/inject.h"
#include "game/inventory/inventory_vars.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/music.h"
#include "game/objects/creatures/pierre.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/room.h"
#include "game/setup.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/stats.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/benchmark.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>
#include <libtrx/virtual_file.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static LEVEL_INFO m_LevelInfo = { 0 };
static INJECTION_INFO *m_InjectionInfo = NULL;

static void Level_LoadFromFile(
    const char *filename, int32_t level_num, bool is_demo);
static void Level_LoadTexturePages(VFILE *file);
static void Level_LoadRooms(VFILE *file);
static void Level_LoadMeshBase(VFILE *file);
static void Level_LoadMeshes(VFILE *file);
static void Level_LoadAnims(VFILE *file);
static void Level_LoadAnimChanges(VFILE *file);
static void Level_LoadAnimRanges(VFILE *file);
static void Level_LoadAnimCommands(VFILE *file);
static void Level_LoadAnimBones(VFILE *file);
static void Level_LoadAnimFrames(VFILE *file);
static void Level_LoadObjects(VFILE *file);
static void Level_LoadStaticObjects(VFILE *file);
static void Level_LoadTextures(VFILE *file);
static void Level_LoadSprites(VFILE *file);
static void Level_LoadCameras(VFILE *file);
static void Level_LoadSoundEffects(VFILE *file);
static void Level_LoadBoxes(VFILE *file);
static void Level_LoadAnimatedTextures(VFILE *file);
static void Level_LoadItems(VFILE *file);
static void Level_LoadDepthQ(VFILE *file);
static void Level_LoadPalette(VFILE *file);
static void Level_LoadCinematic(VFILE *file);
static void Level_LoadDemo(VFILE *file);
static void Level_LoadSamples(VFILE *file);
static void Level_CompleteSetup(int32_t level_num);
static size_t Level_CalculateMaxVertices(void);

static void Level_LoadFromFile(
    const char *filename, int32_t level_num, bool is_demo)
{
    GameBuf_Reset();

    VFILE *file = VFile_CreateFromPath(filename);
    if (!file) {
        Shell_ExitSystemFmt(
            "Level_LoadFromFile(): Could not open %s", filename);
    }

    const int32_t version = VFile_ReadS32(file);
    if (version != 32) {
        Shell_ExitSystemFmt(
            "Level %d (%s) is version %d (this game code is version %d)",
            level_num, filename, version, 32);
    }

    Level_LoadTexturePages(file);

    const int32_t file_level_num = VFile_ReadS32(file);
    LOG_INFO("file level num: %d", file_level_num);

    Level_LoadRooms(file);
    Level_LoadMeshBase(file);
    Level_LoadMeshes(file);
    Level_LoadAnims(file);
    Level_LoadAnimChanges(file);
    Level_LoadAnimRanges(file);
    Level_LoadAnimCommands(file);
    Level_LoadAnimBones(file);
    Level_LoadAnimFrames(file);
    Level_LoadObjects(file);
    Level_LoadStaticObjects(file);
    Level_LoadTextures(file);
    Level_LoadSprites(file);

    if (is_demo) {
        Level_LoadPalette(file);
    }

    Level_LoadCameras(file);
    Level_LoadSoundEffects(file);
    Level_LoadBoxes(file);
    Level_LoadAnimatedTextures(file);
    Level_LoadItems(file);
    Stats_ObserveItemsLoad();
    Level_LoadDepthQ(file);

    if (!is_demo) {
        Level_LoadPalette(file);
    }

    Level_LoadCinematic(file);
    Level_LoadDemo(file);
    Level_LoadSamples(file);

    VFile_Close(file);
}

static void Level_LoadTexturePages(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.texture_page_count = VFile_ReadS32(file);
    LOG_INFO("%d texture pages", m_LevelInfo.texture_page_count);
    m_LevelInfo.texture_page_ptrs =
        Memory_Alloc(m_LevelInfo.texture_page_count * PAGE_SIZE);
    VFile_Read(
        file, m_LevelInfo.texture_page_ptrs,
        PAGE_SIZE * m_LevelInfo.texture_page_count);
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadRooms(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    g_RoomCount = VFile_ReadU16(file);
    LOG_INFO("%d rooms", g_RoomCount);

    g_RoomInfo =
        GameBuf_Alloc(sizeof(ROOM_INFO) * g_RoomCount, GBUF_ROOM_INFOS);
    int i = 0;
    for (ROOM_INFO *current_room_info = g_RoomInfo; i < g_RoomCount;
         i++, current_room_info++) {
        // Room position
        current_room_info->x = VFile_ReadS32(file);
        current_room_info->y = 0;
        current_room_info->z = VFile_ReadS32(file);

        // Room floor/ceiling
        current_room_info->min_floor = VFile_ReadS32(file);
        current_room_info->max_ceiling = VFile_ReadS32(file);

        // Room mesh
        const uint32_t num_meshes = VFile_ReadS32(file);
        const uint32_t inj_mesh_size = Inject_GetExtraRoomMeshSize(i);
        current_room_info->data = GameBuf_Alloc(
            sizeof(uint16_t) * (num_meshes + inj_mesh_size), GBUF_ROOM_MESH);
        VFile_Read(
            file, current_room_info->data, sizeof(uint16_t) * num_meshes);

        // Doors
        const uint16_t num_doors = VFile_ReadS16(file);
        if (!num_doors) {
            current_room_info->doors = NULL;
        } else {
            current_room_info->doors = GameBuf_Alloc(
                sizeof(uint16_t) + sizeof(DOOR_INFO) * num_doors,
                GBUF_ROOM_DOOR);
            current_room_info->doors->count = num_doors;
            for (int32_t j = 0; j < num_doors; j++) {
                DOOR_INFO *door = &current_room_info->doors->door[j];
                door->room_num = VFile_ReadS16(file);
                door->normal.x = VFile_ReadS16(file);
                door->normal.y = VFile_ReadS16(file);
                door->normal.z = VFile_ReadS16(file);
                for (int32_t k = 0; k < 4; k++) {
                    door->vertex[k].x = VFile_ReadS16(file);
                    door->vertex[k].y = VFile_ReadS16(file);
                    door->vertex[k].z = VFile_ReadS16(file);
                }
            }
        }

        // Room floor
        current_room_info->z_size = VFile_ReadS16(file);
        current_room_info->x_size = VFile_ReadS16(file);
        const int32_t sector_count =
            current_room_info->x_size * current_room_info->z_size;
        current_room_info->sectors =
            GameBuf_Alloc(sizeof(SECTOR_INFO) * sector_count, GBUF_ROOM_SECTOR);
        for (int32_t j = 0; j < sector_count; j++) {
            SECTOR_INFO *const sector = &current_room_info->sectors[j];
            sector->index = VFile_ReadU16(file);
            sector->box = VFile_ReadS16(file);
            sector->portal_room.pit = VFile_ReadU8(file);
            const int8_t floor_clicks = VFile_ReadS8(file);
            sector->portal_room.sky = VFile_ReadU8(file);
            const int8_t ceiling_clicks = VFile_ReadS8(file);

            sector->floor.height = floor_clicks * STEP_L;
            sector->ceiling.height = ceiling_clicks * STEP_L;
        }

        // Room lights
        current_room_info->ambient = VFile_ReadS16(file);
        current_room_info->num_lights = VFile_ReadS16(file);
        if (!current_room_info->num_lights) {
            current_room_info->light = NULL;
        } else {
            current_room_info->light = GameBuf_Alloc(
                sizeof(LIGHT_INFO) * current_room_info->num_lights,
                GBUF_ROOM_LIGHTS);
            for (int32_t j = 0; j < current_room_info->num_lights; j++) {
                LIGHT_INFO *light = &current_room_info->light[j];
                light->pos.x = VFile_ReadS32(file);
                light->pos.y = VFile_ReadS32(file);
                light->pos.z = VFile_ReadS32(file);
                light->intensity = VFile_ReadS16(file);
                light->falloff = VFile_ReadS32(file);
            }
        }

        // Static mesh infos
        current_room_info->num_meshes = VFile_ReadS16(file);
        if (!current_room_info->num_meshes) {
            current_room_info->mesh = NULL;
        } else {
            current_room_info->mesh = GameBuf_Alloc(
                sizeof(MESH_INFO) * current_room_info->num_meshes,
                GBUF_ROOM_STATIC_MESH_INFOS);
            for (int32_t j = 0; j < current_room_info->num_meshes; j++) {
                MESH_INFO *mesh = &current_room_info->mesh[j];
                mesh->pos.x = VFile_ReadS32(file);
                mesh->pos.y = VFile_ReadS32(file);
                mesh->pos.z = VFile_ReadS32(file);
                mesh->rot.y = VFile_ReadS16(file);
                mesh->shade = VFile_ReadU16(file);
                mesh->static_number = VFile_ReadU16(file);
            }
        }

        // Flipped (alternative) room
        current_room_info->flipped_room = VFile_ReadS16(file);

        // Room flags
        current_room_info->flags = VFile_ReadU16(file);

        // Initialise some variables
        current_room_info->bound_active = 0;
        current_room_info->left = Viewport_GetMaxX();
        current_room_info->top = Viewport_GetMaxY();
        current_room_info->bottom = 0;
        current_room_info->right = 0;
        current_room_info->item_number = NO_ITEM;
        current_room_info->fx_number = NO_ITEM;
    }

    const int32_t fd_length = VFile_ReadS32(file);
    m_LevelInfo.floor_data = Memory_Alloc(sizeof(int16_t) * fd_length);
    VFile_Read(file, m_LevelInfo.floor_data, sizeof(int16_t) * fd_length);
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadMeshBase(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.mesh_count = VFile_ReadS32(file);
    LOG_INFO("%d meshes", m_LevelInfo.mesh_count);
    g_MeshBase = GameBuf_Alloc(
        sizeof(int16_t)
            * (m_LevelInfo.mesh_count + m_InjectionInfo->mesh_count),
        GBUF_MESHES);
    VFile_Read(file, g_MeshBase, sizeof(int16_t) * m_LevelInfo.mesh_count);
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadMeshes(VFILE *const file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.mesh_ptr_count = VFile_ReadS32(file);
    int32_t *mesh_indices = GameBuf_Alloc(
        sizeof(int32_t) * m_LevelInfo.mesh_ptr_count, GBUF_MESH_POINTERS);
    VFile_Read(
        file, mesh_indices, sizeof(int32_t) * m_LevelInfo.mesh_ptr_count);

    g_Meshes = GameBuf_Alloc(
        sizeof(int16_t *)
            * (m_LevelInfo.mesh_ptr_count + m_InjectionInfo->mesh_ptr_count),
        GBUF_MESH_POINTERS);
    for (int i = 0; i < m_LevelInfo.mesh_ptr_count; i++) {
        g_Meshes[i] = &g_MeshBase[mesh_indices[i] / 2];
    }
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadAnims(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.anim_count = VFile_ReadS32(file);
    LOG_INFO("%d anims", m_LevelInfo.anim_count);
    g_Anims = GameBuf_Alloc(
        sizeof(ANIM_STRUCT)
            * (m_LevelInfo.anim_count + m_InjectionInfo->anim_count),
        GBUF_ANIMS);
    for (int i = 0; i < m_LevelInfo.anim_count; i++) {
        ANIM_STRUCT *anim = g_Anims + i;

        anim->frame_ofs = VFile_ReadU32(file);
        anim->interpolation = VFile_ReadS16(file);
        anim->current_anim_state = VFile_ReadS16(file);
        anim->velocity = VFile_ReadS32(file);
        anim->acceleration = VFile_ReadS32(file);
        anim->frame_base = VFile_ReadS16(file);
        anim->frame_end = VFile_ReadS16(file);
        anim->jump_anim_num = VFile_ReadS16(file);
        anim->jump_frame_num = VFile_ReadS16(file);
        anim->number_changes = VFile_ReadS16(file);
        anim->change_index = VFile_ReadS16(file);
        anim->number_commands = VFile_ReadS16(file);
        anim->command_index = VFile_ReadS16(file);
    }
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadAnimChanges(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.anim_change_count = VFile_ReadS32(file);
    LOG_INFO("%d anim changes", m_LevelInfo.anim_change_count);
    g_AnimChanges = GameBuf_Alloc(
        sizeof(ANIM_CHANGE_STRUCT)
            * (m_LevelInfo.anim_change_count
               + m_InjectionInfo->anim_change_count),
        GBUF_ANIM_CHANGES);
    for (int32_t i = 0; i < m_LevelInfo.anim_change_count; i++) {
        ANIM_CHANGE_STRUCT *anim_change = &g_AnimChanges[i];
        anim_change->goal_anim_state = VFile_ReadS16(file);
        anim_change->number_ranges = VFile_ReadS16(file);
        anim_change->range_index = VFile_ReadS16(file);
    }
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadAnimRanges(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.anim_range_count = VFile_ReadS32(file);
    LOG_INFO("%d anim ranges", m_LevelInfo.anim_range_count);
    g_AnimRanges = GameBuf_Alloc(
        sizeof(ANIM_RANGE_STRUCT)
            * (m_LevelInfo.anim_range_count
               + m_InjectionInfo->anim_range_count),
        GBUF_ANIM_RANGES);
    for (int32_t i = 0; i < m_LevelInfo.anim_range_count; i++) {
        ANIM_RANGE_STRUCT *anim_range = &g_AnimRanges[i];
        anim_range->start_frame = VFile_ReadS16(file);
        anim_range->end_frame = VFile_ReadS16(file);
        anim_range->link_anim_num = VFile_ReadS16(file);
        anim_range->link_frame_num = VFile_ReadS16(file);
    }
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadAnimCommands(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.anim_command_count = VFile_ReadS32(file);
    LOG_INFO("%d anim commands", m_LevelInfo.anim_command_count);
    g_AnimCommands = GameBuf_Alloc(
        sizeof(int16_t)
            * (m_LevelInfo.anim_command_count
               + m_InjectionInfo->anim_cmd_count),
        GBUF_ANIM_COMMANDS);
    VFile_Read(
        file, g_AnimCommands, sizeof(int16_t) * m_LevelInfo.anim_command_count);
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadAnimBones(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.anim_bone_count = VFile_ReadS32(file);
    LOG_INFO("%d anim bones", m_LevelInfo.anim_bone_count);
    g_AnimBones = GameBuf_Alloc(
        sizeof(int32_t)
            * (m_LevelInfo.anim_bone_count + m_InjectionInfo->anim_bone_count),
        GBUF_ANIM_BONES);
    VFile_Read(
        file, g_AnimBones, sizeof(int32_t) * m_LevelInfo.anim_bone_count);
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadAnimFrames(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.anim_frame_data_count = VFile_ReadS32(file);
    LOG_INFO("%d anim frames data", m_LevelInfo.anim_frame_data_count);

    const int32_t raw_data_size = m_LevelInfo.anim_frame_data_count;
    int16_t *raw_data = Memory_Alloc(sizeof(int16_t) * raw_data_size);
    VFile_Read(file, raw_data, sizeof(int16_t) * raw_data_size);

    m_LevelInfo.anim_frame_count = 0;
    m_LevelInfo.anim_frame_mesh_rot_count = 0;
    int16_t *raw_data_ptr = raw_data;
    while (raw_data_ptr - raw_data < raw_data_size) {
        raw_data_ptr += 9;
        const int16_t num_meshes = *raw_data_ptr++;
        raw_data_ptr += num_meshes * sizeof(int32_t) / sizeof(int16_t);
        m_LevelInfo.anim_frame_count++;
        m_LevelInfo.anim_frame_mesh_rot_count += num_meshes;
    }

    LOG_INFO("%d anim frames", m_LevelInfo.anim_frame_count);
    LOG_INFO(
        "%d anim frame mesh rotations", m_LevelInfo.anim_frame_mesh_rot_count);

    g_AnimFrameMeshRots = GameBuf_Alloc(
        sizeof(int32_t)
            * (m_LevelInfo.anim_frame_mesh_rot_count
               + m_InjectionInfo->anim_frame_mesh_rot_count),
        GBUF_ANIM_FRAMES);
    g_AnimFrames = GameBuf_Alloc(
        sizeof(FRAME_INFO)
            * (m_LevelInfo.anim_frame_count
               + m_InjectionInfo->anim_frame_count),
        GBUF_ANIM_FRAMES);
    m_LevelInfo.anim_frame_offsets = Memory_Alloc(
        sizeof(int32_t)
        * (m_LevelInfo.anim_frame_count + m_InjectionInfo->anim_frame_count));

    raw_data_ptr = raw_data;
    int32_t *mesh_rots = g_AnimFrameMeshRots;
    for (int32_t i = 0; i < m_LevelInfo.anim_frame_count; i++) {
        m_LevelInfo.anim_frame_offsets[i] =
            (raw_data_ptr - raw_data) * sizeof(int16_t);
        FRAME_INFO *const frame = &g_AnimFrames[i];
        frame->bounds.min.x = *raw_data_ptr++;
        frame->bounds.max.x = *raw_data_ptr++;
        frame->bounds.min.y = *raw_data_ptr++;
        frame->bounds.max.y = *raw_data_ptr++;
        frame->bounds.min.z = *raw_data_ptr++;
        frame->bounds.max.z = *raw_data_ptr++;
        frame->offset.x = *raw_data_ptr++;
        frame->offset.y = *raw_data_ptr++;
        frame->offset.z = *raw_data_ptr++;
        frame->nmeshes = *raw_data_ptr++;
        frame->mesh_rots = mesh_rots;
        memcpy(mesh_rots, raw_data_ptr, sizeof(int32_t) * frame->nmeshes);
        raw_data_ptr += frame->nmeshes * sizeof(int32_t) / sizeof(int16_t);
        mesh_rots += frame->nmeshes;
    }
    Memory_FreePointer(&raw_data);

    for (int i = 0; i < m_LevelInfo.anim_count; i++) {
        ANIM_STRUCT *anim = &g_Anims[i];
        bool found = false;
        for (int j = 0; j < m_LevelInfo.anim_frame_count; j++) {
            if (m_LevelInfo.anim_frame_offsets[j] == (signed)anim->frame_ofs) {
                anim->frame_ptr = &g_AnimFrames[j];
                found = true;
                break;
            }
        }
        assert(found);
    }

    Benchmark_End(benchmark, NULL);
}

static void Level_LoadObjects(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.object_count = VFile_ReadS32(file);
    LOG_INFO("%d objects", m_LevelInfo.object_count);
    for (int i = 0; i < m_LevelInfo.object_count; i++) {
        const int32_t object_num = VFile_ReadS32(file);
        OBJECT_INFO *object = &g_Objects[object_num];

        object->nmeshes = VFile_ReadS16(file);
        object->mesh_index = VFile_ReadS16(file);
        object->bone_index = VFile_ReadS32(file);

        const int32_t frame_offset = VFile_ReadS32(file);
        object->anim_index = VFile_ReadS16(file);

        bool found = false;
        for (int j = 0; j < m_LevelInfo.anim_frame_count; j++) {
            if (m_LevelInfo.anim_frame_offsets[j] == frame_offset) {
                object->frame_base = &g_AnimFrames[j];
                found = true;
                break;
            }
        }
        object->loaded = found;
    }

    Benchmark_End(benchmark, NULL);
}

static void Level_LoadStaticObjects(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.static_count = VFile_ReadS32(file);
    LOG_INFO("%d statics", m_LevelInfo.static_count);
    for (int i = 0; i < m_LevelInfo.static_count; i++) {
        const int32_t tmp = VFile_ReadS32(file);
        STATIC_INFO *object = &g_StaticObjects[tmp];

        object->mesh_number = VFile_ReadS16(file);
        object->p.min.x = VFile_ReadS16(file);
        object->p.max.x = VFile_ReadS16(file);
        object->p.min.y = VFile_ReadS16(file);
        object->p.max.y = VFile_ReadS16(file);
        object->p.min.z = VFile_ReadS16(file);
        object->p.max.z = VFile_ReadS16(file);
        object->c.min.x = VFile_ReadS16(file);
        object->c.max.x = VFile_ReadS16(file);
        object->c.min.y = VFile_ReadS16(file);
        object->c.max.y = VFile_ReadS16(file);
        object->c.min.z = VFile_ReadS16(file);
        object->c.max.z = VFile_ReadS16(file);
        object->flags = VFile_ReadS16(file);
        object->loaded = true;
    }

    Benchmark_End(benchmark, NULL);
}

static void Level_LoadTextures(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.texture_count = VFile_ReadS32(file);
    LOG_INFO("%d textures", m_LevelInfo.texture_count);
    if ((m_LevelInfo.texture_count + m_InjectionInfo->texture_count)
        > MAX_TEXTURES) {
        Shell_ExitSystem("Too many textures in level");
    }
    for (int32_t i = 0; i < m_LevelInfo.texture_count; i++) {
        PHD_TEXTURE *texture = &g_PhdTextureInfo[i];
        texture->drawtype = VFile_ReadU16(file);
        texture->tpage = VFile_ReadU16(file);
        for (int32_t j = 0; j < 4; j++) {
            texture->uv[j].u = VFile_ReadU16(file);
            texture->uv[j].v = VFile_ReadU16(file);
        }
    }
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadSprites(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.sprite_info_count = VFile_ReadS32(file);
    if (m_LevelInfo.sprite_info_count + m_InjectionInfo->sprite_info_count
        > MAX_SPRITES) {
        Shell_ExitSystem("Too many sprites in level");
    }
    for (int32_t i = 0; i < m_LevelInfo.sprite_info_count; i++) {
        PHD_SPRITE *sprite = &g_PhdSpriteInfo[i];
        sprite->tpage = VFile_ReadU16(file);
        sprite->offset = VFile_ReadU16(file);
        sprite->width = VFile_ReadU16(file);
        sprite->height = VFile_ReadU16(file);
        sprite->x1 = VFile_ReadS16(file);
        sprite->y1 = VFile_ReadS16(file);
        sprite->x2 = VFile_ReadS16(file);
        sprite->y2 = VFile_ReadS16(file);
    }

    m_LevelInfo.sprite_count = VFile_ReadS32(file);
    for (int i = 0; i < m_LevelInfo.sprite_count; i++) {
        GAME_OBJECT_ID object_num;
        object_num = VFile_ReadS32(file);
        const int16_t num_meshes = VFile_ReadS16(file);
        const int16_t mesh_index = VFile_ReadS16(file);

        if (object_num < O_NUMBER_OF) {
            OBJECT_INFO *object = &g_Objects[object_num];
            object->nmeshes = num_meshes;
            object->mesh_index = mesh_index;
            object->loaded = 1;
        } else if (object_num - O_NUMBER_OF < STATIC_NUMBER_OF) {
            STATIC_INFO *object = &g_StaticObjects[object_num - O_NUMBER_OF];
            object->nmeshes = num_meshes;
            object->mesh_number = mesh_index;
            object->loaded = true;
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void Level_LoadCameras(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    g_NumberCameras = VFile_ReadS32(file);
    LOG_INFO("%d cameras", g_NumberCameras);
    if (!g_NumberCameras) {
        return;
    }
    g_Camera.fixed =
        GameBuf_Alloc(sizeof(OBJECT_VECTOR) * g_NumberCameras, GBUF_CAMERAS);
    if (!g_Camera.fixed) {
        Shell_ExitSystem("Error allocating the fixed cameras.");
    }
    for (int32_t i = 0; i < g_NumberCameras; i++) {
        OBJECT_VECTOR *camera = &g_Camera.fixed[i];
        camera->x = VFile_ReadS32(file);
        camera->y = VFile_ReadS32(file);
        camera->z = VFile_ReadS32(file);
        camera->data = VFile_ReadS16(file);
        camera->flags = VFile_ReadS16(file);
    }
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadSoundEffects(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    g_NumberSoundEffects = VFile_ReadS32(file);
    LOG_INFO("%d sound effects", g_NumberSoundEffects);
    if (!g_NumberSoundEffects) {
        return;
    }
    g_SoundEffectsTable = GameBuf_Alloc(
        sizeof(OBJECT_VECTOR) * g_NumberSoundEffects, GBUF_SOUND_FX);
    if (!g_SoundEffectsTable) {
        Shell_ExitSystem("Error allocating the sound effects table.");
    }
    for (int32_t i = 0; i < g_NumberSoundEffects; i++) {
        OBJECT_VECTOR *sound = &g_SoundEffectsTable[i];
        sound->x = VFile_ReadS32(file);
        sound->y = VFile_ReadS32(file);
        sound->z = VFile_ReadS32(file);
        sound->data = VFile_ReadS16(file);
        sound->flags = VFile_ReadS16(file);
    }
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadBoxes(VFILE *file)
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

    m_LevelInfo.overlap_count = VFile_ReadS32(file);
    g_Overlap = GameBuf_Alloc(
        sizeof(uint16_t) * m_LevelInfo.overlap_count, GBUF_OVERLAPS);
    VFile_Read(file, g_Overlap, sizeof(uint16_t) * m_LevelInfo.overlap_count);

    for (int i = 0; i < 2; i++) {
        g_GroundZone[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_GROUNDZONE);
        VFile_Read(file, g_GroundZone[i], sizeof(int16_t) * g_NumberBoxes);

        g_GroundZone2[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_GROUNDZONE);
        VFile_Read(file, g_GroundZone2[i], sizeof(int16_t) * g_NumberBoxes);

        g_FlyZone[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_FLYZONE);
        VFile_Read(file, g_FlyZone[i], sizeof(int16_t) * g_NumberBoxes);
    }

    Benchmark_End(benchmark, NULL);
}

static void Level_LoadAnimatedTextures(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.anim_texture_range_count = VFile_ReadS32(file);
    size_t end_position = VFile_GetPos(file)
        + m_LevelInfo.anim_texture_range_count * sizeof(int16_t);

    const int16_t num_ranges = VFile_ReadS16(file);
    LOG_INFO("%d animated texture ranges", num_ranges);
    if (!num_ranges) {
        g_AnimTextureRanges = NULL;
        goto cleanup;
    }

    g_AnimTextureRanges = GameBuf_Alloc(
        sizeof(TEXTURE_RANGE) * num_ranges, GBUF_ANIMATING_TEXTURE_RANGES);
    for (int32_t i = 0; i < num_ranges; i++) {
        TEXTURE_RANGE *range = &g_AnimTextureRanges[i];
        range->next_range =
            i == num_ranges - 1 ? NULL : &g_AnimTextureRanges[i + 1];

        // Level data is tied to the original logic in Output_AnimateTextures
        // and hence stores one less than the actual count here.
        range->num_textures = VFile_ReadS16(file);
        range->num_textures++;

        range->textures = GameBuf_Alloc(
            sizeof(int16_t) * range->num_textures,
            GBUF_ANIMATING_TEXTURE_RANGES);
        VFile_Read(
            file, range->textures, sizeof(int16_t) * range->num_textures);
    }

cleanup: {
    // Ensure to read everything intended by the level compiler, even if it
    // does not wholly contain accurate texture data.
    const int32_t skip_length = end_position - VFile_GetPos(file);
    if (skip_length > 0) {
        VFile_Skip(file, skip_length);
    }
    Benchmark_End(benchmark, NULL);
}
}

static void Level_LoadItems(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    m_LevelInfo.item_count = VFile_ReadS32(file);

    LOG_INFO("%d items", m_LevelInfo.item_count);

    if (m_LevelInfo.item_count) {
        if (m_LevelInfo.item_count > MAX_ITEMS) {
            Shell_ExitSystem(
                "Level_LoadItems(): Too Many g_Items being Loaded!!");
        }

        g_Items = GameBuf_Alloc(sizeof(ITEM_INFO) * MAX_ITEMS, GBUF_ITEMS);
        g_LevelItemCount = m_LevelInfo.item_count;
        Item_InitialiseArray(MAX_ITEMS);

        for (int i = 0; i < m_LevelInfo.item_count; i++) {
            ITEM_INFO *item = &g_Items[i];
            item->object_number = VFile_ReadS16(file);
            item->room_number = VFile_ReadS16(file);
            item->pos.x = VFile_ReadS32(file);
            item->pos.y = VFile_ReadS32(file);
            item->pos.z = VFile_ReadS32(file);
            item->rot.y = VFile_ReadS16(file);
            item->shade = VFile_ReadS16(file);
            item->flags = VFile_ReadU16(file);

            if (item->object_number < 0 || item->object_number >= O_NUMBER_OF) {
                Shell_ExitSystemFmt(
                    "Level_LoadItems(): Bad Object number (%d) on Item %d",
                    item->object_number, i);
            }
        }
    }

    Benchmark_End(benchmark, NULL);
}

static void Level_LoadDepthQ(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LOG_INFO("");
    VFile_Skip(file, sizeof(uint8_t) * 32 * 256);
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadPalette(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LOG_INFO("");
    RGB_888 palette[256];
    for (int32_t i = 0; i < 256; i++) {
        palette[i].r = VFile_ReadU8(file);
        palette[i].g = VFile_ReadU8(file);
        palette[i].b = VFile_ReadU8(file);
    }
    palette[0].r = 0;
    palette[0].g = 0;
    palette[0].b = 0;
    for (int i = 1; i < 256; i++) {
        palette[i].r *= 4;
        palette[i].g *= 4;
        palette[i].b *= 4;
    }
    Output_SetPalette(palette);
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadCinematic(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    g_NumCineFrames = VFile_ReadS16(file);
    LOG_INFO("%d cinematic frames", g_NumCineFrames);
    if (!g_NumCineFrames) {
        return;
    }
    g_CineCamera = GameBuf_Alloc(
        sizeof(CINE_CAMERA) * g_NumCineFrames, GBUF_CINEMATIC_FRAMES);
    for (int32_t i = 0; i < g_NumCineFrames; i++) {
        CINE_CAMERA *camera = &g_CineCamera[i];
        camera->tx = VFile_ReadS16(file);
        camera->ty = VFile_ReadS16(file);
        camera->tz = VFile_ReadS16(file);
        camera->cx = VFile_ReadS16(file);
        camera->cy = VFile_ReadS16(file);
        camera->cz = VFile_ReadS16(file);
        camera->fov = VFile_ReadS16(file);
        camera->roll = VFile_ReadS16(file);
    }

    Benchmark_End(benchmark, NULL);
}

static void Level_LoadDemo(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    g_DemoData =
        GameBuf_Alloc(sizeof(uint32_t) * DEMO_COUNT_MAX, GBUF_LOADDEMO_BUFFER);
    const uint16_t size = VFile_ReadS16(file);
    LOG_INFO("%d demo buffer size", size);
    if (!size) {
        return;
    }
    VFile_Read(file, g_DemoData, size);
    Benchmark_End(benchmark, NULL);
}

static void Level_LoadSamples(VFILE *file)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    VFile_Read(file, g_SampleLUT, sizeof(int16_t) * MAX_SAMPLES);
    m_LevelInfo.sample_info_count = VFile_ReadS32(file);
    LOG_INFO("%d sample infos", m_LevelInfo.sample_info_count);
    if (!m_LevelInfo.sample_info_count) {
        Shell_ExitSystem("No Sample Infos");
    }

    g_SampleInfos = GameBuf_Alloc(
        sizeof(SAMPLE_INFO)
            * (m_LevelInfo.sample_info_count + m_InjectionInfo->sfx_count),
        GBUF_SAMPLE_INFOS);
    for (int32_t i = 0; i < m_LevelInfo.sample_info_count; i++) {
        SAMPLE_INFO *sample_info = &g_SampleInfos[i];
        sample_info->number = VFile_ReadS16(file);
        sample_info->volume = VFile_ReadS16(file);
        sample_info->randomness = VFile_ReadS16(file);
        sample_info->flags = VFile_ReadS16(file);
    }

    m_LevelInfo.sample_data_size = VFile_ReadS32(file);
    LOG_INFO("%d sample data size", m_LevelInfo.sample_data_size);
    if (!m_LevelInfo.sample_data_size) {
        Shell_ExitSystem("No Sample Data");
    }

    m_LevelInfo.sample_data = GameBuf_Alloc(
        m_LevelInfo.sample_data_size + m_InjectionInfo->sfx_data_size,
        GBUF_SAMPLES);
    VFile_Read(
        file, m_LevelInfo.sample_data,
        sizeof(char) * m_LevelInfo.sample_data_size);

    m_LevelInfo.sample_count = VFile_ReadS32(file);
    LOG_INFO("%d samples", m_LevelInfo.sample_count);
    if (!m_LevelInfo.sample_count) {
        Shell_ExitSystem("No Samples");
    }

    m_LevelInfo.sample_offsets = Memory_Alloc(
        sizeof(int32_t)
        * (m_LevelInfo.sample_count + m_InjectionInfo->sample_count));
    VFile_Read(
        file, m_LevelInfo.sample_offsets,
        sizeof(int32_t) * m_LevelInfo.sample_count);

    Benchmark_End(benchmark, NULL);
}

static void Level_CompleteSetup(int32_t level_num)
{
    BENCHMARK *const benchmark = Benchmark_Start();

    // Expand raw floor data into sectors
    Room_ParseFloorData(m_LevelInfo.floor_data);
    Memory_FreePointer(&m_LevelInfo.floor_data);

    Inject_AllInjections(&m_LevelInfo);

    // Must be called post-injection to allow for floor data changes.
    Stats_ObserveRoomsLoad();

    // Must be called after all g_Anims, g_Meshes etc initialised.
    Setup_AllObjects();

    // Must be called after Setup_AllObjects using the cached item
    // count, as individual setups may increment g_LevelItemCount.
    for (int i = 0; i < m_LevelInfo.item_count; i++) {
        Item_Initialise(i);
    }

    // Configure enemies who carry and drop items
    Carrier_InitialiseLevel(level_num);

    const size_t max_vertices = Level_CalculateMaxVertices();
    LOG_INFO("Maximum vertices: %d", max_vertices);
    Output_ReserveVertexBuffer(max_vertices);

    // Move the prepared texture pages into g_TexturePagePtrs.
    uint8_t *base = GameBuf_Alloc(
        m_LevelInfo.texture_page_count * PAGE_SIZE, GBUF_TEXTURE_PAGES);
    for (int i = 0; i < m_LevelInfo.texture_page_count; i++) {
        g_TexturePagePtrs[i] = base;
        memcpy(base, m_LevelInfo.texture_page_ptrs + i * PAGE_SIZE, PAGE_SIZE);
        base += PAGE_SIZE;
    }
    Output_DownloadTextures(m_LevelInfo.texture_page_count);

    // Initialise the sound effects.
    size_t *sample_sizes =
        Memory_Alloc(sizeof(size_t) * m_LevelInfo.sample_count);
    const char **sample_pointers =
        Memory_Alloc(sizeof(char *) * m_LevelInfo.sample_count);
    for (int i = 0; i < m_LevelInfo.sample_count; i++) {
        sample_pointers[i] =
            m_LevelInfo.sample_data + m_LevelInfo.sample_offsets[i];
    }

    // NOTE: this assumes that sample pointers are sorted
    for (int i = 0; i < m_LevelInfo.sample_count; i++) {
        int current_offset = m_LevelInfo.sample_offsets[i];
        int next_offset = i + 1 >= m_LevelInfo.sample_count
            ? m_LevelInfo.sample_data_size
            : m_LevelInfo.sample_offsets[i + 1];
        sample_sizes[i] = next_offset - current_offset;
    }

    Sound_LoadSamples(m_LevelInfo.sample_count, sample_pointers, sample_sizes);

    Memory_FreePointer(&sample_pointers);
    Memory_FreePointer(&sample_sizes);

    Benchmark_End(benchmark, NULL);
}

static size_t Level_CalculateMaxVertices(void)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    size_t max_vertices = 0;
    for (int32_t i = 0; i < O_NUMBER_OF; i++) {
        const OBJECT_INFO *object_info = &g_Objects[i];
        if (!object_info->loaded) {
            continue;
        }

        for (int32_t j = 0; j < object_info->nmeshes; j++) {
            max_vertices =
                MAX(max_vertices, *(g_Meshes[object_info->mesh_index + j] + 5));
        }
    }

    for (int32_t i = 0; i < STATIC_NUMBER_OF; i++) {
        const STATIC_INFO *static_info = &g_StaticObjects[i];
        if (!static_info->loaded || static_info->nmeshes < 0) {
            continue;
        }

        max_vertices =
            MAX(max_vertices, *(g_Meshes[static_info->mesh_number] + 5));
    }

    for (int32_t i = 0; i < g_RoomCount; i++) {
        max_vertices = MAX(max_vertices, *g_RoomInfo[i].data);
    }

    Benchmark_End(benchmark, NULL);
    return max_vertices;
}

void Level_Load(int level_num)
{
    LOG_INFO("%d (%s)", level_num, g_GameFlow.levels[level_num].level_file);
    BENCHMARK *const benchmark = Benchmark_Start();

    // clean previous level data
    Memory_FreePointer(&m_LevelInfo.texture_page_ptrs);
    Memory_FreePointer(&m_LevelInfo.anim_frame_offsets);
    Memory_FreePointer(&m_LevelInfo.sample_offsets);
    Memory_FreePointer(&m_InjectionInfo);

    m_InjectionInfo = Memory_Alloc(sizeof(INJECTION_INFO));
    Inject_Init(
        g_GameFlow.levels[level_num].injections.length,
        g_GameFlow.levels[level_num].injections.data_paths, m_InjectionInfo);

    bool is_demo =
        (g_GameFlow.levels[level_num].level_type == GFL_TITLE_DEMO_PC)
        | (g_GameFlow.levels[level_num].level_type == GFL_LEVEL_DEMO_PC);

    Level_LoadFromFile(
        g_GameFlow.levels[level_num].level_file, level_num, is_demo);
    Level_CompleteSetup(level_num);

    Inject_Cleanup();

    Output_SetWaterColor(
        g_GameFlow.levels[level_num].water_color.override
            ? &g_GameFlow.levels[level_num].water_color.value
            : &g_GameFlow.water_color);

    Output_SetDrawDistFade(
        (g_GameFlow.levels[level_num].draw_distance_fade.override
             ? g_GameFlow.levels[level_num].draw_distance_fade.value
             : g_GameFlow.draw_distance_fade)
        * WALL_L);

    Output_SetDrawDistMax(
        (g_GameFlow.levels[level_num].draw_distance_max.override
             ? g_GameFlow.levels[level_num].draw_distance_max.value
             : g_GameFlow.draw_distance_max)
        * WALL_L);

    Output_SetSkyboxEnabled(
        g_Config.enable_skybox && g_Objects[O_SKYBOX].loaded);

    Benchmark_End(benchmark, NULL);
}

bool Level_Initialise(int32_t level_num)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    LOG_DEBUG("%d", level_num);

    // loading a save can override it to false
    g_GameInfo.death_counter_supported = true;

    g_GameInfo.select_level_num = -1;
    g_GameInfo.current[level_num].stats.timer = 0;
    g_GameInfo.current[level_num].stats.secret_flags = 0;
    g_GameInfo.current[level_num].stats.pickup_count = 0;
    g_GameInfo.current[level_num].stats.kill_count = 0;
    g_GameInfo.current[level_num].stats.death_count = 0;

    g_LevelComplete = false;
    g_CurrentLevel = level_num;
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
        g_Objects[i].loaded = 0;
    }
    for (int32_t i = 0; i < STATIC_NUMBER_OF; i++) {
        g_StaticObjects[i].loaded = false;
    }

    Camera_Reset();
    Pierre_Reset();

    Lara_InitialiseLoad(NO_ITEM);
    Output_LoadBackdropImage(
        level_num == g_GameFlow.title_level_num
            ? g_GameFlow.main_menu_background_path
            : NULL);

    Level_Load(level_num);

    if (g_Lara.item_number != NO_ITEM) {
        Lara_Initialise(level_num);
    }

    g_Effects = GameBuf_Alloc(NUM_EFFECTS * sizeof(FX_INFO), GBUF_EFFECTS);
    Effect_InitialiseArray();
    LOT_InitialiseArray();

    Overlay_Init();
    Overlay_BarSetHealthTimer(100);

    Music_Stop();
    Sound_ResetEffects();

    Viewport_SetFOV(Viewport_GetUserFOV());

    if (g_GameFlow.levels[level_num].music) {
        Music_PlayLooped(g_GameFlow.levels[level_num].music);
    }

    g_InvItemPuzzle1.string = g_GameFlow.levels[level_num].puzzle1;
    g_InvItemPuzzle2.string = g_GameFlow.levels[level_num].puzzle2;
    g_InvItemPuzzle3.string = g_GameFlow.levels[level_num].puzzle3;
    g_InvItemPuzzle4.string = g_GameFlow.levels[level_num].puzzle4;
    g_InvItemKey1.string = g_GameFlow.levels[level_num].key1;
    g_InvItemKey2.string = g_GameFlow.levels[level_num].key2;
    g_InvItemKey3.string = g_GameFlow.levels[level_num].key3;
    g_InvItemKey4.string = g_GameFlow.levels[level_num].key4;
    g_InvItemPickup1.string = g_GameFlow.levels[level_num].pickup1;
    g_InvItemPickup2.string = g_GameFlow.levels[level_num].pickup2;

    g_Camera.underwater = false;
    Benchmark_End(benchmark, NULL);
    return true;
}

const LEVEL_INFO *Level_GetInfo(void)
{
    return &m_LevelInfo;
}
