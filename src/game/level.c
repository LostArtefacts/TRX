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

#include <libtrx/filesystem.h>
#include <libtrx/log.h>
#include <libtrx/memory.h>
#include <libtrx/utils.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static LEVEL_INFO m_LevelInfo = { 0 };
static INJECTION_INFO *m_InjectionInfo = NULL;

static bool Level_LoadRooms(MYFILE *fp);
static bool Level_LoadObjects(MYFILE *fp);
static bool Level_LoadSprites(MYFILE *fp);
static bool Level_LoadItems(MYFILE *fp);
static bool Level_LoadDepthQ(MYFILE *fp);
static bool Level_LoadPalette(MYFILE *fp);
static bool Level_LoadCameras(MYFILE *fp);
static bool Level_LoadSoundEffects(MYFILE *fp);
static bool Level_LoadBoxes(MYFILE *fp);
static bool Level_LoadAnimatedTextures(MYFILE *fp);
static bool Level_LoadCinematic(MYFILE *fp);
static bool Level_LoadDemo(MYFILE *fp);
static bool Level_LoadSamples(MYFILE *fp);
static bool Level_LoadTexturePages(MYFILE *fp);

static bool Level_LoadFromFile(
    const char *filename, int32_t level_num, bool is_demo);
static void Level_CompleteSetup(int32_t level_num);
static size_t Level_CalculateMaxVertices(void);

static bool Level_LoadFromFile(
    const char *filename, int32_t level_num, bool is_demo)
{
    GameBuf_Shutdown();
    GameBuf_Init();

    MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
    if (!fp) {
        Shell_ExitSystemFmt(
            "Level_LoadFromFile(): Could not open %s", filename);
        return false;
    }

    const int32_t version = File_ReadS32(fp);
    if (version != 32) {
        Shell_ExitSystemFmt(
            "Level %d (%s) is version %d (this game code is version %d)",
            level_num, filename, version, 32);
        return false;
    }

    if (!Level_LoadTexturePages(fp)) {
        return false;
    }

    const int32_t file_level_num = File_ReadS32(fp);
    LOG_INFO("file level num: %d", file_level_num);

    if (!Level_LoadRooms(fp)) {
        return false;
    }

    if (!Level_LoadObjects(fp)) {
        return false;
    }

    if (!Level_LoadSprites(fp)) {
        return false;
    }

    if (is_demo) {
        if (!Level_LoadPalette(fp)) {
            return false;
        }
    }

    if (!Level_LoadCameras(fp)) {
        return false;
    }

    if (!Level_LoadSoundEffects(fp)) {
        return false;
    }

    if (!Level_LoadBoxes(fp)) {
        return false;
    }

    if (!Level_LoadAnimatedTextures(fp)) {
        return false;
    }

    if (!Level_LoadItems(fp)) {
        return false;
    }
    Stats_ObserveItemsLoad();

    if (!Level_LoadDepthQ(fp)) {
        return false;
    }

    if (!is_demo) {
        if (!Level_LoadPalette(fp)) {
            return false;
        }
    }

    if (!Level_LoadCinematic(fp)) {
        return false;
    }

    if (!Level_LoadDemo(fp)) {
        return false;
    }

    if (!Level_LoadSamples(fp)) {
        return false;
    }

    File_Close(fp);

    return true;
}

static bool Level_LoadRooms(MYFILE *fp)
{
    g_RoomCount = File_ReadU16(fp);
    LOG_INFO("%d rooms", g_RoomCount);

    g_RoomInfo =
        GameBuf_Alloc(sizeof(ROOM_INFO) * g_RoomCount, GBUF_ROOM_INFOS);
    int i = 0;
    for (ROOM_INFO *current_room_info = g_RoomInfo; i < g_RoomCount;
         i++, current_room_info++) {
        // Room position
        current_room_info->x = File_ReadS32(fp);
        current_room_info->y = 0;
        current_room_info->z = File_ReadS32(fp);

        // Room floor/ceiling
        current_room_info->min_floor = File_ReadS32(fp);
        current_room_info->max_ceiling = File_ReadS32(fp);

        // Room mesh
        const uint32_t num_meshes = File_ReadS32(fp);
        const uint32_t inj_mesh_size = Inject_GetExtraRoomMeshSize(i);
        current_room_info->data = GameBuf_Alloc(
            sizeof(uint16_t) * (num_meshes + inj_mesh_size), GBUF_ROOM_MESH);
        File_ReadItems(
            fp, current_room_info->data, sizeof(uint16_t), num_meshes);

        // Doors
        const uint16_t num_doors = File_ReadS16(fp);
        if (!num_doors) {
            current_room_info->doors = NULL;
        } else {
            current_room_info->doors = GameBuf_Alloc(
                sizeof(uint16_t) + sizeof(DOOR_INFO) * num_doors,
                GBUF_ROOM_DOOR);
            current_room_info->doors->count = num_doors;
            for (int32_t j = 0; j < num_doors; j++) {
                DOOR_INFO *door = &current_room_info->doors->door[j];
                door->room_num = File_ReadS16(fp);
                door->normal.x = File_ReadS16(fp);
                door->normal.y = File_ReadS16(fp);
                door->normal.z = File_ReadS16(fp);
                for (int32_t k = 0; k < 4; k++) {
                    door->vertex[k].x = File_ReadS16(fp);
                    door->vertex[k].y = File_ReadS16(fp);
                    door->vertex[k].z = File_ReadS16(fp);
                }
            }
        }

        // Room floor
        current_room_info->z_size = File_ReadS16(fp);
        current_room_info->x_size = File_ReadS16(fp);
        const int32_t sector_count =
            current_room_info->x_size * current_room_info->z_size;
        current_room_info->sectors =
            GameBuf_Alloc(sizeof(SECTOR_INFO) * sector_count, GBUF_ROOM_SECTOR);
        for (int32_t j = 0; j < sector_count; j++) {
            SECTOR_INFO *const sector = &current_room_info->sectors[j];
            sector->index = File_ReadU16(fp);
            sector->box = File_ReadS16(fp);
            sector->portal_room.pit = File_ReadU8(fp);
            const int8_t floor_clicks = File_ReadS8(fp);
            sector->portal_room.sky = File_ReadU8(fp);
            const int8_t ceiling_clicks = File_ReadS8(fp);

            sector->floor.height = floor_clicks * STEP_L;
            sector->ceiling.height = ceiling_clicks * STEP_L;
        }

        // Room lights
        current_room_info->ambient = File_ReadS16(fp);
        current_room_info->num_lights = File_ReadS16(fp);
        if (!current_room_info->num_lights) {
            current_room_info->light = NULL;
        } else {
            current_room_info->light = GameBuf_Alloc(
                sizeof(LIGHT_INFO) * current_room_info->num_lights,
                GBUF_ROOM_LIGHTS);
            for (int32_t j = 0; j < current_room_info->num_lights; j++) {
                LIGHT_INFO *light = &current_room_info->light[j];
                light->pos.x = File_ReadS32(fp);
                light->pos.y = File_ReadS32(fp);
                light->pos.z = File_ReadS32(fp);
                light->intensity = File_ReadS16(fp);
                light->falloff = File_ReadS32(fp);
            }
        }

        // Static mesh infos
        current_room_info->num_meshes = File_ReadS16(fp);
        if (!current_room_info->num_meshes) {
            current_room_info->mesh = NULL;
        } else {
            current_room_info->mesh = GameBuf_Alloc(
                sizeof(MESH_INFO) * current_room_info->num_meshes,
                GBUF_ROOM_STATIC_MESH_INFOS);
            for (int32_t j = 0; j < current_room_info->num_meshes; j++) {
                MESH_INFO *mesh = &current_room_info->mesh[j];
                mesh->pos.x = File_ReadS32(fp);
                mesh->pos.y = File_ReadS32(fp);
                mesh->pos.z = File_ReadS32(fp);
                mesh->rot.y = File_ReadS16(fp);
                mesh->shade = File_ReadU16(fp);
                mesh->static_number = File_ReadU16(fp);
            }
        }

        // Flipped (alternative) room
        current_room_info->flipped_room = File_ReadS16(fp);

        // Room flags
        current_room_info->flags = File_ReadU16(fp);

        // Initialise some variables
        current_room_info->bound_active = 0;
        current_room_info->left = Viewport_GetMaxX();
        current_room_info->top = Viewport_GetMaxY();
        current_room_info->bottom = 0;
        current_room_info->right = 0;
        current_room_info->item_number = NO_ITEM;
        current_room_info->fx_number = NO_ITEM;
    }

    m_LevelInfo.floor_data_size = File_ReadS32(fp);
    g_FloorData = GameBuf_Alloc(
        sizeof(int16_t)
            * (m_LevelInfo.floor_data_size + m_InjectionInfo->floor_data_size),
        GBUF_FLOOR_DATA);
    File_ReadItems(
        fp, g_FloorData, sizeof(int16_t), m_LevelInfo.floor_data_size);

    return true;
}

static bool Level_LoadObjects(MYFILE *fp)
{
    m_LevelInfo.mesh_count = File_ReadS32(fp);
    LOG_INFO("%d meshes", m_LevelInfo.mesh_count);
    g_MeshBase = GameBuf_Alloc(
        sizeof(int16_t)
            * (m_LevelInfo.mesh_count + m_InjectionInfo->mesh_count),
        GBUF_MESHES);
    File_ReadItems(fp, g_MeshBase, sizeof(int16_t), m_LevelInfo.mesh_count);

    m_LevelInfo.mesh_ptr_count = File_ReadS32(fp);
    int32_t *mesh_indices = GameBuf_Alloc(
        sizeof(int32_t) * m_LevelInfo.mesh_ptr_count, GBUF_MESH_POINTERS);
    File_ReadItems(
        fp, mesh_indices, sizeof(int32_t), m_LevelInfo.mesh_ptr_count);

    g_Meshes = GameBuf_Alloc(
        sizeof(int16_t *)
            * (m_LevelInfo.mesh_ptr_count + m_InjectionInfo->mesh_ptr_count),
        GBUF_MESH_POINTERS);
    for (int i = 0; i < m_LevelInfo.mesh_ptr_count; i++) {
        g_Meshes[i] = &g_MeshBase[mesh_indices[i] / 2];
    }

    m_LevelInfo.anim_count = File_ReadS32(fp);
    LOG_INFO("%d anims", m_LevelInfo.anim_count);
    g_Anims = GameBuf_Alloc(
        sizeof(ANIM_STRUCT)
            * (m_LevelInfo.anim_count + m_InjectionInfo->anim_count),
        GBUF_ANIMS);
    for (int i = 0; i < m_LevelInfo.anim_count; i++) {
        ANIM_STRUCT *anim = g_Anims + i;

        anim->frame_ofs = File_ReadU32(fp);
        anim->interpolation = File_ReadS16(fp);
        anim->current_anim_state = File_ReadS16(fp);
        anim->velocity = File_ReadS32(fp);
        anim->acceleration = File_ReadS32(fp);
        anim->frame_base = File_ReadS16(fp);
        anim->frame_end = File_ReadS16(fp);
        anim->jump_anim_num = File_ReadS16(fp);
        anim->jump_frame_num = File_ReadS16(fp);
        anim->number_changes = File_ReadS16(fp);
        anim->change_index = File_ReadS16(fp);
        anim->number_commands = File_ReadS16(fp);
        anim->command_index = File_ReadS16(fp);
    }

    m_LevelInfo.anim_change_count = File_ReadS32(fp);
    LOG_INFO("%d anim changes", m_LevelInfo.anim_change_count);
    g_AnimChanges = GameBuf_Alloc(
        sizeof(ANIM_CHANGE_STRUCT)
            * (m_LevelInfo.anim_change_count
               + m_InjectionInfo->anim_change_count),
        GBUF_ANIM_CHANGES);
    for (int32_t i = 0; i < m_LevelInfo.anim_change_count; i++) {
        ANIM_CHANGE_STRUCT *anim_change = &g_AnimChanges[i];
        anim_change->goal_anim_state = File_ReadS16(fp);
        anim_change->number_ranges = File_ReadS16(fp);
        anim_change->range_index = File_ReadS16(fp);
    }

    m_LevelInfo.anim_range_count = File_ReadS32(fp);
    LOG_INFO("%d anim ranges", m_LevelInfo.anim_range_count);
    g_AnimRanges = GameBuf_Alloc(
        sizeof(ANIM_RANGE_STRUCT)
            * (m_LevelInfo.anim_range_count
               + m_InjectionInfo->anim_range_count),
        GBUF_ANIM_RANGES);
    for (int32_t i = 0; i < m_LevelInfo.anim_range_count; i++) {
        ANIM_RANGE_STRUCT *anim_range = &g_AnimRanges[i];
        anim_range->start_frame = File_ReadS16(fp);
        anim_range->end_frame = File_ReadS16(fp);
        anim_range->link_anim_num = File_ReadS16(fp);
        anim_range->link_frame_num = File_ReadS16(fp);
    }

    m_LevelInfo.anim_command_count = File_ReadS32(fp);
    LOG_INFO("%d anim commands", m_LevelInfo.anim_command_count);
    g_AnimCommands = GameBuf_Alloc(
        sizeof(int16_t)
            * (m_LevelInfo.anim_command_count
               + m_InjectionInfo->anim_cmd_count),
        GBUF_ANIM_COMMANDS);
    File_ReadItems(
        fp, g_AnimCommands, sizeof(int16_t), m_LevelInfo.anim_command_count);

    m_LevelInfo.anim_bone_count = File_ReadS32(fp);
    LOG_INFO("%d anim bones", m_LevelInfo.anim_bone_count);
    g_AnimBones = GameBuf_Alloc(
        sizeof(int32_t)
            * (m_LevelInfo.anim_bone_count + m_InjectionInfo->anim_bone_count),
        GBUF_ANIM_BONES);
    File_ReadItems(
        fp, g_AnimBones, sizeof(int32_t), m_LevelInfo.anim_bone_count);

    m_LevelInfo.anim_frame_data_count = File_ReadS32(fp);
    LOG_INFO("%d anim frames data", m_LevelInfo.anim_frame_data_count);

    const size_t frame_data_start = File_Pos(fp);
    File_Skip(fp, m_LevelInfo.anim_frame_data_count * sizeof(int16_t));
    const size_t frame_data_end = File_Pos(fp);

    m_LevelInfo.anim_frame_count = 0;
    m_LevelInfo.anim_frame_mesh_rot_count = 0;
    File_Seek(fp, frame_data_start, SEEK_SET);
    while (File_Pos(fp) < frame_data_end) {
        File_Skip(fp, 9 * sizeof(int16_t));
        const int16_t num_meshes = File_ReadS16(fp);
        File_Skip(fp, num_meshes * sizeof(int32_t));
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

    File_Seek(fp, frame_data_start, SEEK_SET);
    int32_t *mesh_rots = g_AnimFrameMeshRots;
    for (int32_t i = 0; i < m_LevelInfo.anim_frame_count; i++) {
        m_LevelInfo.anim_frame_offsets[i] = File_Pos(fp) - frame_data_start;
        FRAME_INFO *const frame = &g_AnimFrames[i];
        frame->bounds.min.x = File_ReadS16(fp);
        frame->bounds.max.x = File_ReadS16(fp);
        frame->bounds.min.y = File_ReadS16(fp);
        frame->bounds.max.y = File_ReadS16(fp);
        frame->bounds.min.z = File_ReadS16(fp);
        frame->bounds.max.z = File_ReadS16(fp);
        frame->offset.x = File_ReadS16(fp);
        frame->offset.y = File_ReadS16(fp);
        frame->offset.z = File_ReadS16(fp);
        frame->nmeshes = File_ReadS16(fp);
        frame->mesh_rots = mesh_rots;
        File_ReadItems(fp, mesh_rots, sizeof(int32_t), frame->nmeshes);
        mesh_rots += frame->nmeshes;
    }
    assert(File_Pos(fp) == frame_data_end);

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
    File_Seek(fp, frame_data_end, SEEK_SET);

    m_LevelInfo.object_count = File_ReadS32(fp);
    LOG_INFO("%d objects", m_LevelInfo.object_count);
    for (int i = 0; i < m_LevelInfo.object_count; i++) {
        const int32_t object_num = File_ReadS32(fp);
        OBJECT_INFO *object = &g_Objects[object_num];

        object->nmeshes = File_ReadS16(fp);
        object->mesh_index = File_ReadS16(fp);
        object->bone_index = File_ReadS32(fp);

        const int32_t frame_offset = File_ReadS32(fp);
        object->anim_index = File_ReadS16(fp);

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

    m_LevelInfo.static_count = File_ReadS32(fp);
    LOG_INFO("%d statics", m_LevelInfo.static_count);
    for (int i = 0; i < m_LevelInfo.static_count; i++) {
        const int32_t tmp = File_ReadS32(fp);
        STATIC_INFO *object = &g_StaticObjects[tmp];

        object->mesh_number = File_ReadS16(fp);
        object->p.min.x = File_ReadS16(fp);
        object->p.max.x = File_ReadS16(fp);
        object->p.min.y = File_ReadS16(fp);
        object->p.max.y = File_ReadS16(fp);
        object->p.min.z = File_ReadS16(fp);
        object->p.max.z = File_ReadS16(fp);
        object->c.min.x = File_ReadS16(fp);
        object->c.max.x = File_ReadS16(fp);
        object->c.min.y = File_ReadS16(fp);
        object->c.max.y = File_ReadS16(fp);
        object->c.min.z = File_ReadS16(fp);
        object->c.max.z = File_ReadS16(fp);
        object->flags = File_ReadS16(fp);
        object->loaded = true;
    }

    m_LevelInfo.texture_count = File_ReadS32(fp);
    LOG_INFO("%d textures", m_LevelInfo.texture_count);
    if ((m_LevelInfo.texture_count + m_InjectionInfo->texture_count)
        > MAX_TEXTURES) {
        return false;
    }
    for (int32_t i = 0; i < m_LevelInfo.texture_count; i++) {
        PHD_TEXTURE *texture = &g_PhdTextureInfo[i];
        texture->drawtype = File_ReadU16(fp);
        texture->tpage = File_ReadU16(fp);
        for (int32_t j = 0; j < 4; j++) {
            texture->uv[j].u = File_ReadU16(fp);
            texture->uv[j].v = File_ReadU16(fp);
        }
    }

    return true;
}

static bool Level_LoadSprites(MYFILE *fp)
{
    m_LevelInfo.sprite_info_count = File_ReadS32(fp);
    if (m_LevelInfo.sprite_info_count + m_InjectionInfo->sprite_info_count
        > MAX_SPRITES) {
        Shell_ExitSystem("Too many sprites in level");
        return false;
    }
    for (int32_t i = 0; i < m_LevelInfo.sprite_info_count; i++) {
        PHD_SPRITE *sprite = &g_PhdSpriteInfo[i];
        sprite->tpage = File_ReadU16(fp);
        sprite->offset = File_ReadU16(fp);
        sprite->width = File_ReadU16(fp);
        sprite->height = File_ReadU16(fp);
        sprite->x1 = File_ReadS16(fp);
        sprite->y1 = File_ReadS16(fp);
        sprite->x2 = File_ReadS16(fp);
        sprite->y2 = File_ReadS16(fp);
    }

    m_LevelInfo.sprite_count = File_ReadS32(fp);
    for (int i = 0; i < m_LevelInfo.sprite_count; i++) {
        GAME_OBJECT_ID object_num;
        object_num = File_ReadS32(fp);
        const int16_t num_meshes = File_ReadS16(fp);
        const int16_t mesh_index = File_ReadS16(fp);

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
    return true;
}

static bool Level_LoadItems(MYFILE *fp)
{
    m_LevelInfo.item_count = File_ReadS32(fp);

    LOG_INFO("%d items", m_LevelInfo.item_count);

    if (m_LevelInfo.item_count) {
        if (m_LevelInfo.item_count > MAX_ITEMS) {
            Shell_ExitSystem(
                "Level_LoadItems(): Too Many g_Items being Loaded!!");
            return false;
        }

        g_Items = GameBuf_Alloc(sizeof(ITEM_INFO) * MAX_ITEMS, GBUF_ITEMS);
        g_LevelItemCount = m_LevelInfo.item_count;
        Item_InitialiseArray(MAX_ITEMS);

        for (int i = 0; i < m_LevelInfo.item_count; i++) {
            ITEM_INFO *item = &g_Items[i];
            item->object_number = File_ReadS16(fp);
            item->room_number = File_ReadS16(fp);
            item->pos.x = File_ReadS32(fp);
            item->pos.y = File_ReadS32(fp);
            item->pos.z = File_ReadS32(fp);
            item->rot.y = File_ReadS16(fp);
            item->shade = File_ReadS16(fp);
            item->flags = File_ReadU16(fp);

            if (item->object_number < 0 || item->object_number >= O_NUMBER_OF) {
                Shell_ExitSystemFmt(
                    "Level_LoadItems(): Bad Object number (%d) on Item %d",
                    item->object_number, i);
            }
        }
    }

    return true;
}

static bool Level_LoadDepthQ(MYFILE *fp)
{
    LOG_INFO("");
    File_Seek(fp, sizeof(uint8_t) * 32 * 256, FILE_SEEK_CUR);
    return true;
}

static bool Level_LoadPalette(MYFILE *fp)
{
    LOG_INFO("");
    RGB_888 palette[256];
    for (int32_t i = 0; i < 256; i++) {
        palette[i].r = File_ReadU8(fp);
        palette[i].g = File_ReadU8(fp);
        palette[i].b = File_ReadU8(fp);
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
    return true;
}

static bool Level_LoadCameras(MYFILE *fp)
{
    g_NumberCameras = File_ReadS32(fp);
    LOG_INFO("%d cameras", g_NumberCameras);
    if (!g_NumberCameras) {
        return true;
    }
    g_Camera.fixed =
        GameBuf_Alloc(sizeof(OBJECT_VECTOR) * g_NumberCameras, GBUF_CAMERAS);
    if (!g_Camera.fixed) {
        return false;
    }
    for (int32_t i = 0; i < g_NumberCameras; i++) {
        OBJECT_VECTOR *camera = &g_Camera.fixed[i];
        camera->x = File_ReadS32(fp);
        camera->y = File_ReadS32(fp);
        camera->z = File_ReadS32(fp);
        camera->data = File_ReadS16(fp);
        camera->flags = File_ReadS16(fp);
    }
    return true;
}

static bool Level_LoadSoundEffects(MYFILE *fp)
{
    g_NumberSoundEffects = File_ReadS32(fp);
    LOG_INFO("%d sound effects", g_NumberSoundEffects);
    if (!g_NumberSoundEffects) {
        return true;
    }
    g_SoundEffectsTable = GameBuf_Alloc(
        sizeof(OBJECT_VECTOR) * g_NumberSoundEffects, GBUF_SOUND_FX);
    if (!g_SoundEffectsTable) {
        return false;
    }
    for (int32_t i = 0; i < g_NumberSoundEffects; i++) {
        OBJECT_VECTOR *sound = &g_SoundEffectsTable[i];
        sound->x = File_ReadS32(fp);
        sound->y = File_ReadS32(fp);
        sound->z = File_ReadS32(fp);
        sound->data = File_ReadS16(fp);
        sound->flags = File_ReadS16(fp);
    }
    return true;
}

static bool Level_LoadBoxes(MYFILE *fp)
{
    g_NumberBoxes = File_ReadS32(fp);
    g_Boxes = GameBuf_Alloc(sizeof(BOX_INFO) * g_NumberBoxes, GBUF_BOXES);
    for (int32_t i = 0; i < g_NumberBoxes; i++) {
        BOX_INFO *box = &g_Boxes[i];
        box->left = File_ReadS32(fp);
        box->right = File_ReadS32(fp);
        box->top = File_ReadS32(fp);
        box->bottom = File_ReadS32(fp);
        box->height = File_ReadS16(fp);
        box->overlap_index = File_ReadS16(fp);
    }

    m_LevelInfo.overlap_count = File_ReadS32(fp);
    g_Overlap = GameBuf_Alloc(
        sizeof(uint16_t) * m_LevelInfo.overlap_count, GBUF_OVERLAPS);
    File_ReadItems(fp, g_Overlap, sizeof(uint16_t), m_LevelInfo.overlap_count);

    for (int i = 0; i < 2; i++) {
        g_GroundZone[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_GROUNDZONE);
        File_ReadItems(fp, g_GroundZone[i], sizeof(int16_t), g_NumberBoxes);

        g_GroundZone2[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_GROUNDZONE);
        File_ReadItems(fp, g_GroundZone2[i], sizeof(int16_t), g_NumberBoxes);

        g_FlyZone[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_FLYZONE);
        File_ReadItems(fp, g_FlyZone[i], sizeof(int16_t), g_NumberBoxes);
    }

    return true;
}

static bool Level_LoadAnimatedTextures(MYFILE *fp)
{
    m_LevelInfo.anim_texture_range_count = File_ReadS32(fp);
    size_t end_position =
        File_Pos(fp) + m_LevelInfo.anim_texture_range_count * sizeof(int16_t);

    const int16_t num_ranges = File_ReadS16(fp);
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
        range->num_textures = File_ReadS16(fp);
        range->num_textures++;

        range->textures = GameBuf_Alloc(
            sizeof(int16_t) * range->num_textures,
            GBUF_ANIMATING_TEXTURE_RANGES);
        File_ReadItems(
            fp, range->textures, sizeof(int16_t), range->num_textures);
    }

cleanup:
    // Ensure to read everything intended by the level compiler, even if it
    // does not wholly contain accurate texture data.
    File_Seek(fp, MAX(end_position, File_Pos(fp)), SEEK_SET);
    return true;
}

static bool Level_LoadCinematic(MYFILE *fp)
{
    g_NumCineFrames = File_ReadS16(fp);
    LOG_INFO("%d cinematic frames", g_NumCineFrames);
    if (!g_NumCineFrames) {
        return true;
    }
    g_CineCamera = GameBuf_Alloc(
        sizeof(CINE_CAMERA) * g_NumCineFrames, GBUF_CINEMATIC_FRAMES);
    for (int32_t i = 0; i < g_NumCineFrames; i++) {
        CINE_CAMERA *camera = &g_CineCamera[i];
        camera->tx = File_ReadS16(fp);
        camera->ty = File_ReadS16(fp);
        camera->tz = File_ReadS16(fp);
        camera->cx = File_ReadS16(fp);
        camera->cy = File_ReadS16(fp);
        camera->cz = File_ReadS16(fp);
        camera->fov = File_ReadS16(fp);
        camera->roll = File_ReadS16(fp);
    }
    return true;
}

static bool Level_LoadDemo(MYFILE *fp)
{
    g_DemoData =
        GameBuf_Alloc(sizeof(uint32_t) * DEMO_COUNT_MAX, GBUF_LOADDEMO_BUFFER);
    const uint16_t size = File_ReadS16(fp);
    LOG_INFO("%d demo buffer size", size);
    if (!size) {
        return true;
    }
    File_ReadData(fp, g_DemoData, size);
    return true;
}

static bool Level_LoadSamples(MYFILE *fp)
{
    File_ReadItems(fp, g_SampleLUT, sizeof(int16_t), MAX_SAMPLES);
    m_LevelInfo.sample_info_count = File_ReadS32(fp);
    LOG_INFO("%d sample infos", m_LevelInfo.sample_info_count);
    if (!m_LevelInfo.sample_info_count) {
        Shell_ExitSystem("No Sample Infos");
        return false;
    }

    g_SampleInfos = GameBuf_Alloc(
        sizeof(SAMPLE_INFO)
            * (m_LevelInfo.sample_info_count + m_InjectionInfo->sfx_count),
        GBUF_SAMPLE_INFOS);
    for (int32_t i = 0; i < m_LevelInfo.sample_info_count; i++) {
        SAMPLE_INFO *sample_info = &g_SampleInfos[i];
        sample_info->number = File_ReadS16(fp);
        sample_info->volume = File_ReadS16(fp);
        sample_info->randomness = File_ReadS16(fp);
        sample_info->flags = File_ReadS16(fp);
    }

    m_LevelInfo.sample_data_size = File_ReadS32(fp);
    LOG_INFO("%d sample data size", m_LevelInfo.sample_data_size);
    if (!m_LevelInfo.sample_data_size) {
        Shell_ExitSystem("No Sample Data");
        return false;
    }

    m_LevelInfo.sample_data = GameBuf_Alloc(
        m_LevelInfo.sample_data_size + m_InjectionInfo->sfx_data_size,
        GBUF_SAMPLES);
    File_ReadItems(
        fp, m_LevelInfo.sample_data, sizeof(char),
        m_LevelInfo.sample_data_size);

    m_LevelInfo.sample_count = File_ReadS32(fp);
    LOG_INFO("%d samples", m_LevelInfo.sample_count);
    if (!m_LevelInfo.sample_count) {
        Shell_ExitSystem("No Samples");
        return false;
    }

    m_LevelInfo.sample_offsets = Memory_Alloc(
        sizeof(int32_t)
        * (m_LevelInfo.sample_count + m_InjectionInfo->sample_count));
    File_ReadItems(
        fp, m_LevelInfo.sample_offsets, sizeof(int32_t),
        m_LevelInfo.sample_count);

    return true;
}

static bool Level_LoadTexturePages(MYFILE *fp)
{
    m_LevelInfo.texture_page_count = File_ReadS32(fp);
    LOG_INFO("%d texture pages", m_LevelInfo.texture_page_count);
    m_LevelInfo.texture_page_ptrs =
        Memory_Alloc(m_LevelInfo.texture_page_count * PAGE_SIZE);
    File_ReadItems(
        fp, m_LevelInfo.texture_page_ptrs, PAGE_SIZE,
        m_LevelInfo.texture_page_count);
    return true;
}

static void Level_CompleteSetup(int32_t level_num)
{
    Inject_AllInjections(&m_LevelInfo);

    // Expand raw floor data into sectors
    Room_ParseFloorData(g_FloorData);
    // TODO: store raw FD temporarily in m_LevelInfo, release here and eliminate
    // g_FloorData

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
}

static size_t Level_CalculateMaxVertices(void)
{
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

    return max_vertices;
}

bool Level_Load(int level_num)
{
    LOG_INFO("%d (%s)", level_num, g_GameFlow.levels[level_num].level_file);

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

    bool ret = Level_LoadFromFile(
        g_GameFlow.levels[level_num].level_file, level_num, is_demo);

    if (ret) {
        Level_CompleteSetup(level_num);
    }

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

    return ret;
}

bool Level_Initialise(int32_t level_num)
{
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
    Output_ApplyRenderSettings();

    if (!Level_Load(level_num)) {
        return false;
    }

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
    return true;
}

const LEVEL_INFO *Level_GetInfo(void)
{
    return &m_LevelInfo;
}
