#include "game/level.h"

#include "filesystem.h"
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
#include "log.h"
#include "memory.h"

#include <stdio.h>
#include <string.h>

static LEVEL_INFO m_LevelInfo;
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

static bool Level_LoadFromFile(const char *filename, int32_t level_num);
static void Level_CompleteSetup(int32_t level_num);

static bool Level_LoadFromFile(const char *filename, int32_t level_num)
{
    int32_t version;
    int32_t file_level_num;

    GameBuf_Shutdown();
    GameBuf_Init();

    MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
    if (!fp) {
        Shell_ExitSystemFmt(
            "Level_LoadFromFile(): Could not open %s", filename);
        return false;
    }

    File_Read(&version, sizeof(int32_t), 1, fp);
    if (version != 32) {
        Shell_ExitSystemFmt(
            "Level %d (%s) is version %d (this game code is version %d)",
            level_num, filename, version, 32);
        return false;
    }

    if (!Level_LoadTexturePages(fp)) {
        return false;
    }

    File_Read(&file_level_num, sizeof(int32_t), 1, fp);
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

    if (!Level_LoadPalette(fp)) {
        return false;
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
    uint16_t count2;
    uint32_t count4;
    uint32_t inj_mesh_size;

    File_Read(&g_RoomCount, sizeof(uint16_t), 1, fp);
    LOG_INFO("%d rooms", g_RoomCount);

    g_RoomInfo =
        GameBuf_Alloc(sizeof(ROOM_INFO) * g_RoomCount, GBUF_ROOM_INFOS);
    int i = 0;
    for (ROOM_INFO *current_room_info = g_RoomInfo; i < g_RoomCount;
         i++, current_room_info++) {
        // Room position
        File_Read(&current_room_info->x, sizeof(uint32_t), 1, fp);
        current_room_info->y = 0;
        File_Read(&current_room_info->z, sizeof(uint32_t), 1, fp);

        // Room floor/ceiling
        File_Read(&current_room_info->min_floor, sizeof(uint32_t), 1, fp);
        File_Read(&current_room_info->max_ceiling, sizeof(uint32_t), 1, fp);

        // Room mesh
        File_Read(&count4, sizeof(uint32_t), 1, fp);
        inj_mesh_size = Inject_GetExtraRoomMeshSize(i);
        current_room_info->data = GameBuf_Alloc(
            sizeof(uint16_t) * (count4 + inj_mesh_size), GBUF_ROOM_MESH);
        File_Read(current_room_info->data, sizeof(uint16_t), count4, fp);

        // Doors
        File_Read(&count2, sizeof(uint16_t), 1, fp);
        if (!count2) {
            current_room_info->doors = NULL;
        } else {
            current_room_info->doors = GameBuf_Alloc(
                sizeof(uint16_t) + sizeof(DOOR_INFO) * count2, GBUF_ROOM_DOOR);
            current_room_info->doors->count = count2;
            File_Read(
                &current_room_info->doors->door, sizeof(DOOR_INFO), count2, fp);
        }

        // Room floor
        File_Read(&current_room_info->x_size, sizeof(uint16_t), 1, fp);
        File_Read(&current_room_info->y_size, sizeof(uint16_t), 1, fp);
        count4 = current_room_info->y_size * current_room_info->x_size;
        current_room_info->floor =
            GameBuf_Alloc(sizeof(FLOOR_INFO) * count4, GBUF_ROOM_FLOOR);
        File_Read(current_room_info->floor, sizeof(FLOOR_INFO), count4, fp);

        // Room lights
        File_Read(&current_room_info->ambient, sizeof(uint16_t), 1, fp);
        File_Read(&current_room_info->num_lights, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_lights) {
            current_room_info->light = NULL;
        } else {
            current_room_info->light = GameBuf_Alloc(
                sizeof(LIGHT_INFO) * current_room_info->num_lights,
                GBUF_ROOM_LIGHTS);
            File_Read(
                current_room_info->light, sizeof(LIGHT_INFO),
                current_room_info->num_lights, fp);
        }

        // Static mesh infos
        File_Read(&current_room_info->num_meshes, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_meshes) {
            current_room_info->mesh = NULL;
        } else {
            current_room_info->mesh = GameBuf_Alloc(
                sizeof(MESH_INFO) * current_room_info->num_meshes,
                GBUF_ROOM_STATIC_MESH_INFOS);
            File_Read(
                current_room_info->mesh, sizeof(MESH_INFO),
                current_room_info->num_meshes, fp);
        }

        // Flipped (alternative) room
        File_Read(&current_room_info->flipped_room, sizeof(uint16_t), 1, fp);

        // Room flags
        File_Read(&current_room_info->flags, sizeof(uint16_t), 1, fp);

        // Initialise some variables
        current_room_info->bound_active = 0;
        current_room_info->left = Viewport_GetMaxX();
        current_room_info->top = Viewport_GetMaxY();
        current_room_info->bottom = 0;
        current_room_info->right = 0;
        current_room_info->item_number = -1;
        current_room_info->fx_number = -1;
    }

    File_Read(&m_LevelInfo.floor_data_size, sizeof(uint32_t), 1, fp);
    g_FloorData = GameBuf_Alloc(
        sizeof(uint16_t)
            * (m_LevelInfo.floor_data_size + m_InjectionInfo->floor_data_size),
        GBUF_FLOOR_DATA);
    File_Read(g_FloorData, sizeof(uint16_t), m_LevelInfo.floor_data_size, fp);

    return true;
}

static bool Level_LoadObjects(MYFILE *fp)
{
    File_Read(&m_LevelInfo.mesh_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d meshes", m_LevelInfo.mesh_count);
    g_MeshBase = GameBuf_Alloc(
        sizeof(int16_t)
            * (m_LevelInfo.mesh_count + m_InjectionInfo->mesh_count),
        GBUF_MESHES);
    File_Read(g_MeshBase, sizeof(int16_t), m_LevelInfo.mesh_count, fp);

    File_Read(&m_LevelInfo.mesh_ptr_count, sizeof(int32_t), 1, fp);
    uint32_t *mesh_indices = GameBuf_Alloc(
        sizeof(uint32_t) * m_LevelInfo.mesh_ptr_count, GBUF_MESH_POINTERS);
    File_Read(mesh_indices, sizeof(uint32_t), m_LevelInfo.mesh_ptr_count, fp);

    g_Meshes = GameBuf_Alloc(
        sizeof(int16_t *)
            * (m_LevelInfo.mesh_ptr_count + m_InjectionInfo->mesh_ptr_count),
        GBUF_MESH_POINTERS);
    for (int i = 0; i < m_LevelInfo.mesh_ptr_count; i++) {
        g_Meshes[i] = &g_MeshBase[mesh_indices[i] / 2];
    }

    File_Read(&m_LevelInfo.anim_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anims", m_LevelInfo.anim_count);
    g_Anims = GameBuf_Alloc(
        sizeof(ANIM_STRUCT)
            * (m_LevelInfo.anim_count + m_InjectionInfo->anim_count),
        GBUF_ANIMS);
    for (int i = 0; i < m_LevelInfo.anim_count; i++) {
        ANIM_STRUCT *anim = g_Anims + i;

        File_Read(&anim->frame_ofs, sizeof(uint32_t), 1, fp);
        File_Read(&anim->interpolation, sizeof(int16_t), 1, fp);
        File_Read(&anim->current_anim_state, sizeof(int16_t), 1, fp);
        File_Read(&anim->velocity, sizeof(int32_t), 1, fp);
        File_Read(&anim->acceleration, sizeof(int32_t), 1, fp);
        File_Read(&anim->frame_base, sizeof(int16_t), 1, fp);
        File_Read(&anim->frame_end, sizeof(int16_t), 1, fp);
        File_Read(&anim->jump_anim_num, sizeof(int16_t), 1, fp);
        File_Read(&anim->jump_frame_num, sizeof(int16_t), 1, fp);
        File_Read(&anim->number_changes, sizeof(int16_t), 1, fp);
        File_Read(&anim->change_index, sizeof(int16_t), 1, fp);
        File_Read(&anim->number_commands, sizeof(int16_t), 1, fp);
        File_Read(&anim->command_index, sizeof(int16_t), 1, fp);
    }

    File_Read(&m_LevelInfo.anim_change_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim changes", m_LevelInfo.anim_change_count);
    g_AnimChanges = GameBuf_Alloc(
        sizeof(ANIM_CHANGE_STRUCT)
            * (m_LevelInfo.anim_change_count
               + m_InjectionInfo->anim_change_count),
        GBUF_ANIM_CHANGES);
    File_Read(
        g_AnimChanges, sizeof(ANIM_CHANGE_STRUCT),
        m_LevelInfo.anim_change_count, fp);

    File_Read(&m_LevelInfo.anim_range_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim ranges", m_LevelInfo.anim_range_count);
    g_AnimRanges = GameBuf_Alloc(
        sizeof(ANIM_RANGE_STRUCT)
            * (m_LevelInfo.anim_range_count
               + m_InjectionInfo->anim_range_count),
        GBUF_ANIM_RANGES);
    File_Read(
        g_AnimRanges, sizeof(ANIM_RANGE_STRUCT), m_LevelInfo.anim_range_count,
        fp);

    File_Read(&m_LevelInfo.anim_command_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim commands", m_LevelInfo.anim_command_count);
    g_AnimCommands = GameBuf_Alloc(
        sizeof(int16_t)
            * (m_LevelInfo.anim_command_count
               + m_InjectionInfo->anim_cmd_count),
        GBUF_ANIM_COMMANDS);
    File_Read(
        g_AnimCommands, sizeof(int16_t), m_LevelInfo.anim_command_count, fp);

    File_Read(&m_LevelInfo.anim_bone_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim bones", m_LevelInfo.anim_bone_count);
    g_AnimBones = GameBuf_Alloc(
        sizeof(int32_t)
            * (m_LevelInfo.anim_bone_count + m_InjectionInfo->anim_bone_count),
        GBUF_ANIM_BONES);
    File_Read(g_AnimBones, sizeof(int32_t), m_LevelInfo.anim_bone_count, fp);

    File_Read(&m_LevelInfo.anim_frame_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim frames", m_LevelInfo.anim_frame_count);
    g_AnimFrames = GameBuf_Alloc(
        sizeof(int16_t)
            * (m_LevelInfo.anim_frame_count
               + m_InjectionInfo->anim_frame_count),
        GBUF_ANIM_FRAMES);
    File_Read(g_AnimFrames, sizeof(int16_t), m_LevelInfo.anim_frame_count, fp);
    for (int i = 0; i < m_LevelInfo.anim_count; i++) {
        g_Anims[i].frame_ptr = &g_AnimFrames[g_Anims[i].frame_ofs / 2];
    }

    File_Read(&m_LevelInfo.object_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d objects", m_LevelInfo.object_count);
    for (int i = 0; i < m_LevelInfo.object_count; i++) {
        int32_t tmp;
        File_Read(&tmp, sizeof(int32_t), 1, fp);
        OBJECT_INFO *object = &g_Objects[tmp];

        File_Read(&object->nmeshes, sizeof(int16_t), 1, fp);
        File_Read(&object->mesh_index, sizeof(int16_t), 1, fp);
        File_Read(&object->bone_index, sizeof(int32_t), 1, fp);

        File_Read(&tmp, sizeof(int32_t), 1, fp);
        object->frame_base = &g_AnimFrames[tmp / 2];
        File_Read(&object->anim_index, sizeof(int16_t), 1, fp);
        object->loaded = 1;
    }

    File_Read(&m_LevelInfo.static_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d statics", m_LevelInfo.static_count);
    for (int i = 0; i < m_LevelInfo.static_count; i++) {
        int32_t tmp;
        File_Read(&tmp, sizeof(int32_t), 1, fp);
        STATIC_INFO *object = &g_StaticObjects[tmp];

        File_Read(&object->mesh_number, sizeof(int16_t), 1, fp);
        File_Read(&object->x_minp, sizeof(int16_t), 6, fp);
        File_Read(&object->x_minc, sizeof(int16_t), 6, fp);
        File_Read(&object->flags, sizeof(int16_t), 1, fp);
    }

    File_Read(&m_LevelInfo.texture_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d textures", m_LevelInfo.texture_count);
    if ((m_LevelInfo.texture_count + m_InjectionInfo->texture_count)
        > MAX_TEXTURES) {
        return false;
    }
    File_Read(
        g_PhdTextureInfo, sizeof(PHD_TEXTURE), m_LevelInfo.texture_count, fp);

    return true;
}

static bool Level_LoadSprites(MYFILE *fp)
{
    File_Read(&m_LevelInfo.sprite_info_count, sizeof(int32_t), 1, fp);
    if (m_LevelInfo.sprite_info_count + m_InjectionInfo->sprite_info_count
        > MAX_SPRITES) {
        Shell_ExitSystem("Too many sprites in level");
        return false;
    }
    File_Read(
        &g_PhdSpriteInfo, sizeof(PHD_SPRITE), m_LevelInfo.sprite_info_count,
        fp);

    File_Read(&m_LevelInfo.sprite_count, sizeof(int32_t), 1, fp);
    for (int i = 0; i < m_LevelInfo.sprite_count; i++) {
        GAME_OBJECT_ID object_num;
        File_Read(&object_num, sizeof(int32_t), 1, fp);
        if (object_num < O_NUMBER_OF) {
            File_Read(&g_Objects[object_num], sizeof(int16_t), 1, fp);
            File_Read(
                &g_Objects[object_num].mesh_index, sizeof(int16_t), 1, fp);
            g_Objects[object_num].loaded = 1;
        } else {
            int32_t static_num = object_num - O_NUMBER_OF;
            File_Skip(fp, 2);
            File_Read(
                &g_StaticObjects[static_num].mesh_number, sizeof(int16_t), 1,
                fp);
        }
    }
    return true;
}

static bool Level_LoadItems(MYFILE *fp)
{
    File_Read(&m_LevelInfo.item_count, sizeof(int32_t), 1, fp);

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
            File_Read(&item->object_number, sizeof(int16_t), 1, fp);
            File_Read(&item->room_number, sizeof(int16_t), 1, fp);
            File_Read(&item->pos.x, sizeof(int32_t), 1, fp);
            File_Read(&item->pos.y, sizeof(int32_t), 1, fp);
            File_Read(&item->pos.z, sizeof(int32_t), 1, fp);
            File_Read(&item->pos.y_rot, sizeof(int16_t), 1, fp);
            File_Read(&item->shade, sizeof(int16_t), 1, fp);
            File_Read(&item->flags, sizeof(uint16_t), 1, fp);

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
    RGB888 palette[256];
    File_Read(palette, sizeof(RGB888), 256, fp);
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
    File_Read(&g_NumberCameras, sizeof(int32_t), 1, fp);
    LOG_INFO("%d cameras", g_NumberCameras);
    if (!g_NumberCameras) {
        return true;
    }
    g_Camera.fixed =
        GameBuf_Alloc(sizeof(OBJECT_VECTOR) * g_NumberCameras, GBUF_CAMERAS);
    if (!g_Camera.fixed) {
        return false;
    }
    File_Read(g_Camera.fixed, sizeof(OBJECT_VECTOR), g_NumberCameras, fp);
    return true;
}

static bool Level_LoadSoundEffects(MYFILE *fp)
{
    File_Read(&g_NumberSoundEffects, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sound effects", g_NumberSoundEffects);
    if (!g_NumberSoundEffects) {
        return true;
    }
    g_SoundEffectsTable = GameBuf_Alloc(
        sizeof(OBJECT_VECTOR) * g_NumberSoundEffects, GBUF_SOUND_FX);
    if (!g_SoundEffectsTable) {
        return false;
    }
    File_Read(
        g_SoundEffectsTable, sizeof(OBJECT_VECTOR), g_NumberSoundEffects, fp);
    return true;
}

static bool Level_LoadBoxes(MYFILE *fp)
{
    File_Read(&g_NumberBoxes, sizeof(int32_t), 1, fp);
    g_Boxes = GameBuf_Alloc(sizeof(BOX_INFO) * g_NumberBoxes, GBUF_BOXES);
    if (!File_Read(g_Boxes, sizeof(BOX_INFO), g_NumberBoxes, fp)) {
        Shell_ExitSystem("Level_LoadBoxes(): Unable to load boxes");
        return false;
    }

    File_Read(&m_LevelInfo.overlap_count, sizeof(int32_t), 1, fp);
    g_Overlap = GameBuf_Alloc(
        sizeof(uint16_t) * m_LevelInfo.overlap_count, GBUF_OVERLAPS);
    if (!File_Read(
            g_Overlap, sizeof(uint16_t), m_LevelInfo.overlap_count, fp)) {
        Shell_ExitSystem("Level_LoadBoxes(): Unable to load box overlaps");
        return false;
    }

    for (int i = 0; i < 2; i++) {
        g_GroundZone[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_GROUNDZONE);
        if (!g_GroundZone[i]
            || !File_Read(
                g_GroundZone[i], sizeof(int16_t), g_NumberBoxes, fp)) {
            Shell_ExitSystem("Level_LoadBoxes(): Unable to load 'ground_zone'");
            return false;
        }

        g_GroundZone2[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_GROUNDZONE);
        if (!g_GroundZone2[i]
            || !File_Read(
                g_GroundZone2[i], sizeof(int16_t), g_NumberBoxes, fp)) {
            Shell_ExitSystem(
                "Level_LoadBoxes(): Unable to load 'ground2_zone'");
            return false;
        }

        g_FlyZone[i] =
            GameBuf_Alloc(sizeof(int16_t) * g_NumberBoxes, GBUF_FLYZONE);
        if (!g_FlyZone[i]
            || !File_Read(g_FlyZone[i], sizeof(int16_t), g_NumberBoxes, fp)) {
            Shell_ExitSystem("Level_LoadBoxes(): Unable to load 'fly_zone'");
            return false;
        }
    }

    return true;
}

static bool Level_LoadAnimatedTextures(MYFILE *fp)
{
    File_Read(&m_LevelInfo.anim_texture_range_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d animated textures", m_LevelInfo.anim_texture_range_count);
    g_AnimTextureRanges = GameBuf_Alloc(
        sizeof(int16_t) * m_LevelInfo.anim_texture_range_count,
        GBUF_ANIMATING_TEXTURE_RANGES);
    File_Read(
        g_AnimTextureRanges, sizeof(int16_t),
        m_LevelInfo.anim_texture_range_count, fp);
    return true;
}

static bool Level_LoadCinematic(MYFILE *fp)
{
    File_Read(&g_NumCineFrames, sizeof(int16_t), 1, fp);
    LOG_INFO("%d cinematic frames", g_NumCineFrames);
    if (!g_NumCineFrames) {
        return true;
    }
    g_Cine = GameBuf_Alloc(
        sizeof(int16_t) * 8 * g_NumCineFrames, GBUF_CINEMATIC_FRAMES);
    File_Read(g_Cine, sizeof(int16_t) * 8, g_NumCineFrames, fp);
    return true;
}

static bool Level_LoadDemo(MYFILE *fp)
{
    g_DemoData =
        GameBuf_Alloc(sizeof(uint32_t) * DEMO_COUNT_MAX, GBUF_LOADDEMO_BUFFER);
    uint16_t size = 0;
    File_Read(&size, sizeof(int16_t), 1, fp);
    LOG_INFO("%d demo buffer size", size);
    if (!size) {
        return true;
    }
    File_Read(g_DemoData, 1, size, fp);
    return true;
}

static bool Level_LoadSamples(MYFILE *fp)
{
    File_Read(g_SampleLUT, sizeof(int16_t), MAX_SAMPLES, fp);
    File_Read(&m_LevelInfo.sample_info_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sample infos", m_LevelInfo.sample_info_count);
    if (!m_LevelInfo.sample_info_count) {
        Shell_ExitSystem("No Sample Infos");
        return false;
    }

    g_SampleInfos = GameBuf_Alloc(
        sizeof(SAMPLE_INFO)
            * (m_LevelInfo.sample_info_count + m_InjectionInfo->sfx_count),
        GBUF_SAMPLE_INFOS);
    File_Read(
        g_SampleInfos, sizeof(SAMPLE_INFO), m_LevelInfo.sample_info_count, fp);

    File_Read(&m_LevelInfo.sample_data_size, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sample data size", m_LevelInfo.sample_data_size);
    if (!m_LevelInfo.sample_data_size) {
        Shell_ExitSystem("No Sample Data");
        return false;
    }

    m_LevelInfo.sample_data = GameBuf_Alloc(
        m_LevelInfo.sample_data_size + m_InjectionInfo->sfx_data_size,
        GBUF_SAMPLES);
    File_Read(
        m_LevelInfo.sample_data, sizeof(char), m_LevelInfo.sample_data_size,
        fp);

    File_Read(&m_LevelInfo.sample_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d samples", m_LevelInfo.sample_count);
    if (!m_LevelInfo.sample_count) {
        Shell_ExitSystem("No Samples");
        return false;
    }

    m_LevelInfo.sample_offsets = Memory_Alloc(
        sizeof(int32_t)
        * (m_LevelInfo.sample_count + m_InjectionInfo->sample_count));
    File_Read(
        m_LevelInfo.sample_offsets, sizeof(int32_t), m_LevelInfo.sample_count,
        fp);

    return true;
}

static bool Level_LoadTexturePages(MYFILE *fp)
{
    File_Read(&m_LevelInfo.texture_page_count, sizeof(int32_t), 1, fp);
    LOG_INFO("%d texture pages", m_LevelInfo.texture_page_count);
    m_LevelInfo.texture_page_ptrs =
        Memory_Alloc(m_LevelInfo.texture_page_count * PAGE_SIZE);
    File_Read(
        m_LevelInfo.texture_page_ptrs, PAGE_SIZE,
        m_LevelInfo.texture_page_count, fp);
    return true;
}

static void Level_CompleteSetup(int32_t level_num)
{
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

bool Level_Load(int level_num)
{
    LOG_INFO("%d (%s)", level_num, g_GameFlow.levels[level_num].level_file);

    m_InjectionInfo = Memory_Alloc(sizeof(INJECTION_INFO));
    Inject_Init(
        g_GameFlow.levels[level_num].injections.length,
        g_GameFlow.levels[level_num].injections.data_paths, m_InjectionInfo);

    bool ret =
        Level_LoadFromFile(g_GameFlow.levels[level_num].level_file, level_num);

    if (ret) {
        Level_CompleteSetup(level_num);
    }

    Inject_Cleanup();

    Memory_FreePointer(&m_LevelInfo.texture_page_ptrs);
    Memory_FreePointer(&m_LevelInfo.sample_offsets);
    Memory_FreePointer(&m_InjectionInfo);

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
    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        g_FlipMapTable[i] = 0;
    }

    for (int i = 0; i < MAX_CD_TRACKS; i++) {
        g_MusicTrackFlags[i] = 0;
    }

    /* Clear Object Loaded flags */
    for (int i = 0; i < O_NUMBER_OF; i++) {
        g_Objects[i].loaded = 0;
    }

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

    g_Camera.underwater = 0;
    return true;
}
