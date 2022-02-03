#include "game/level.h"

#include "config.h"
#include "filesystem.h"
#include "game/control.h"
#include "game/gamebuf.h"
#include "game/gameflow.h"
#include "game/items.h"
#include "game/output.h"
#include "game/setup.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "log.h"
#include "memory.h"

#include <stdio.h>

static int32_t m_MeshCount = 0;
static int32_t m_MeshPtrCount = 0;
static int32_t m_AnimCount = 0;
static int32_t m_AnimChangeCount = 0;
static int32_t m_AnimRangeCount = 0;
static int32_t m_AnimCommandCount = 0;
static int32_t m_AnimBoneCount = 0;
static int32_t m_AnimFrameCount = 0;
static int32_t m_ObjectCount = 0;
static int32_t m_StaticCount = 0;
static int32_t m_TextureCount = 0;
static int32_t m_FloorDataSize = 0;
static int32_t m_TexturePageCount = 0;
static int32_t m_AnimTextureRangeCount = 0;
static int32_t m_SpriteInfoCount = 0;
static int32_t m_SpriteCount = 0;
static int32_t m_OverlapCount = 0;
static int32_t m_LevelPickups = 0;
static int32_t m_LevelKillables = 0;

// TODO: game doesn't add O_SCION_ITEM4 (complete Scion Atlantis) as pickup
int16_t m_PickupObjs[] = { O_PICKUP_ITEM1,   O_PICKUP_ITEM2,  O_KEY_ITEM1,
                           O_KEY_ITEM2,      O_KEY_ITEM3,     O_KEY_ITEM4,
                           O_PUZZLE_ITEM1,   O_PUZZLE_ITEM2,  O_PUZZLE_ITEM3,
                           O_PUZZLE_ITEM4,   O_GUN_ITEM,      O_SHOTGUN_ITEM,
                           O_MAGNUM_ITEM,    O_UZI_ITEM,      O_GUN_AMMO_ITEM,
                           O_SG_AMMO_ITEM,   O_MAG_AMMO_ITEM, O_UZI_AMMO_ITEM,
                           O_EXPLOSIVE_ITEM, O_MEDI_ITEM,     O_BIGMEDI_ITEM,
                           O_SCION_ITEM,     O_SCION_ITEM2,   O_LEADBAR_ITEM,
                           NO_ITEM };

// TODO: need to check only active mummies
int16_t m_KillableObjs[] = {
    O_WOLF,       O_BEAR,        O_BAT,          O_CROCODILE,  O_ALLIGATOR,
    O_LION,       O_LIONESS,     O_PUMA,         O_APE,        O_RAT,
    O_VOLE,       O_DINOSAUR,    O_RAPTOR,       O_WARRIOR1,   O_WARRIOR2,
    O_WARRIOR3,   O_CENTAUR,     O_DINO_WARRIOR, O_LARSON,     O_PIERRE,
    O_SKATEBOARD, O_MERCENARY1,  O_MERCENARY2,   O_MERCENARY3, O_NATLA,
    O_ABORTION,   O_SCION_ITEM3, NO_ITEM
};

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

    Output_DownloadTextures(m_TexturePageCount);

    return true;
}

static bool Level_LoadRooms(MYFILE *fp)
{
    uint16_t count2;
    uint32_t count4;

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
        current_room_info->data =
            GameBuf_Alloc(sizeof(uint16_t) * count4, GBUF_ROOM_MESH);
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
        current_room_info->left = ViewPort_GetMaxX();
        current_room_info->top = ViewPort_GetMaxY();
        current_room_info->bottom = 0;
        current_room_info->right = 0;
        current_room_info->item_number = -1;
        current_room_info->fx_number = -1;
    }

    File_Read(&m_FloorDataSize, sizeof(uint32_t), 1, fp);
    g_FloorData =
        GameBuf_Alloc(sizeof(uint16_t) * m_FloorDataSize, GBUF_FLOOR_DATA);
    File_Read(g_FloorData, sizeof(uint16_t), m_FloorDataSize, fp);

    return true;
}

static bool Level_LoadObjects(MYFILE *fp)
{
    File_Read(&m_MeshCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d meshes", m_MeshCount);
    g_MeshBase = GameBuf_Alloc(sizeof(int16_t) * m_MeshCount, GBUF_MESHES);
    File_Read(g_MeshBase, sizeof(int16_t), m_MeshCount, fp);

    File_Read(&m_MeshPtrCount, sizeof(int32_t), 1, fp);
    uint32_t *mesh_indices =
        GameBuf_Alloc(sizeof(uint32_t) * m_MeshPtrCount, GBUF_MESH_POINTERS);
    File_Read(mesh_indices, sizeof(uint32_t), m_MeshPtrCount, fp);

    g_Meshes =
        GameBuf_Alloc(sizeof(int16_t *) * m_MeshPtrCount, GBUF_MESH_POINTERS);
    for (int i = 0; i < m_MeshPtrCount; i++) {
        g_Meshes[i] = &g_MeshBase[mesh_indices[i] / 2];
    }

    File_Read(&m_AnimCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anims", m_AnimCount);
    g_Anims = GameBuf_Alloc(sizeof(ANIM_STRUCT) * m_AnimCount, GBUF_ANIMS);
    File_Read(g_Anims, sizeof(ANIM_STRUCT), m_AnimCount, fp);

    File_Read(&m_AnimChangeCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim changes", m_AnimChangeCount);
    g_AnimChanges = GameBuf_Alloc(
        sizeof(ANIM_CHANGE_STRUCT) * m_AnimChangeCount, GBUF_ANIM_CHANGES);
    File_Read(g_AnimChanges, sizeof(ANIM_CHANGE_STRUCT), m_AnimChangeCount, fp);

    File_Read(&m_AnimRangeCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim ranges", m_AnimRangeCount);
    g_AnimRanges = GameBuf_Alloc(
        sizeof(ANIM_RANGE_STRUCT) * m_AnimRangeCount, GBUF_ANIM_RANGES);
    File_Read(g_AnimRanges, sizeof(ANIM_RANGE_STRUCT), m_AnimRangeCount, fp);

    File_Read(&m_AnimCommandCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim commands", m_AnimCommandCount);
    g_AnimCommands =
        GameBuf_Alloc(sizeof(int16_t) * m_AnimCommandCount, GBUF_ANIM_COMMANDS);
    File_Read(g_AnimCommands, sizeof(int16_t), m_AnimCommandCount, fp);

    File_Read(&m_AnimBoneCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim bones", m_AnimBoneCount);
    g_AnimBones =
        GameBuf_Alloc(sizeof(int32_t) * m_AnimBoneCount, GBUF_ANIM_BONES);
    File_Read(g_AnimBones, sizeof(int32_t), m_AnimBoneCount, fp);

    File_Read(&m_AnimFrameCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim frames", m_AnimFrameCount);
    g_AnimFrames =
        GameBuf_Alloc(sizeof(int16_t) * m_AnimFrameCount, GBUF_ANIM_FRAMES);
    File_Read(g_AnimFrames, sizeof(int16_t), m_AnimFrameCount, fp);
    for (int i = 0; i < m_AnimCount; i++) {
        g_Anims[i].frame_ptr = &g_AnimFrames[(size_t)g_Anims[i].frame_ptr / 2];
    }

    File_Read(&m_ObjectCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d objects", m_ObjectCount);
    for (int i = 0; i < m_ObjectCount; i++) {
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

    InitialiseObjects();

    File_Read(&m_StaticCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d statics", m_StaticCount);
    for (int i = 0; i < m_StaticCount; i++) {
        int32_t tmp;
        File_Read(&tmp, sizeof(int32_t), 1, fp);
        STATIC_INFO *object = &g_StaticObjects[tmp];

        File_Read(&object->mesh_number, sizeof(int16_t), 1, fp);
        File_Read(&object->x_minp, sizeof(int16_t), 6, fp);
        File_Read(&object->x_minc, sizeof(int16_t), 6, fp);
        File_Read(&object->flags, sizeof(int16_t), 1, fp);
    }

    File_Read(&m_TextureCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d textures", m_TextureCount);
    if (m_TextureCount > MAX_TEXTURES) {
        Shell_ExitSystem("Too many Textures in level");
        return false;
    }
    File_Read(g_PhdTextureInfo, sizeof(PHD_TEXTURE), m_TextureCount, fp);

    return true;
}

static bool Level_LoadSprites(MYFILE *fp)
{
    File_Read(&m_SpriteInfoCount, sizeof(int32_t), 1, fp);
    if (m_SpriteInfoCount > MAX_SPRITES) {
        Shell_ExitSystem("Too many sprites in level");
        return false;
    }
    File_Read(&g_PhdSpriteInfo, sizeof(PHD_SPRITE), m_SpriteInfoCount, fp);

    File_Read(&m_SpriteCount, sizeof(int32_t), 1, fp);
    for (int i = 0; i < m_SpriteCount; i++) {
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
    m_LevelPickups = 0;
    m_LevelKillables = 0;
    int32_t item_count = 0;
    File_Read(&item_count, sizeof(int32_t), 1, fp);

    LOG_INFO("%d items", item_count);

    if (item_count) {
        if (item_count > MAX_ITEMS) {
            Shell_ExitSystem(
                "Level_LoadItems(): Too Many g_Items being Loaded!!");
            return false;
        }

        g_Items = GameBuf_Alloc(sizeof(ITEM_INFO) * MAX_ITEMS, GBUF_ITEMS);
        g_LevelItemCount = item_count;
        InitialiseItemArray(MAX_ITEMS);

        for (int i = 0; i < item_count; i++) {
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

            InitialiseItem(i);

            // Calculate number of pickups in a level
            for (int16_t j = 0; m_PickupObjs[j] != NO_ITEM; j++) {
                if (item->object_number == m_PickupObjs[j]) {
                    m_LevelPickups++;
                }
            }

            // Calculate number of killable objects in a level
            for (int16_t j = 0; m_KillableObjs[j] != NO_ITEM; j++) {
                if (item->object_number == m_KillableObjs[j]) {
                    m_LevelKillables++;
                }
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

    File_Read(&m_OverlapCount, sizeof(int32_t), 1, fp);
    g_Overlap = GameBuf_Alloc(sizeof(uint16_t) * m_OverlapCount, 22);
    if (!File_Read(g_Overlap, sizeof(uint16_t), m_OverlapCount, fp)) {
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
    File_Read(&m_AnimTextureRangeCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d animated textures", m_AnimTextureRangeCount);
    g_AnimTextureRanges = GameBuf_Alloc(
        sizeof(int16_t) * m_AnimTextureRangeCount,
        GBUF_ANIMATING_TEXTURE_RANGES);
    File_Read(
        g_AnimTextureRanges, sizeof(int16_t), m_AnimTextureRangeCount, fp);
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
    int32_t num_sample_infos;
    File_Read(&num_sample_infos, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sample infos", num_sample_infos);
    if (!num_sample_infos) {
        Shell_ExitSystem("No Sample Infos");
        return false;
    }

    g_SampleInfos = GameBuf_Alloc(
        sizeof(SAMPLE_INFO) * num_sample_infos, GBUF_SAMPLE_INFOS);
    File_Read(g_SampleInfos, sizeof(SAMPLE_INFO), num_sample_infos, fp);

    int32_t sample_data_size;
    File_Read(&sample_data_size, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sample data size", sample_data_size);
    if (!sample_data_size) {
        Shell_ExitSystem("No Sample Data");
        return false;
    }

    char *sample_data = GameBuf_Alloc(sample_data_size, GBUF_SAMPLES);
    File_Read(sample_data, sizeof(char), sample_data_size, fp);

    int32_t num_samples;
    File_Read(&num_samples, sizeof(int32_t), 1, fp);
    LOG_INFO("%d samples", num_samples);
    if (!num_samples) {
        Shell_ExitSystem("No Samples");
        return false;
    }

    int32_t *sample_offsets = Memory_Alloc(sizeof(int32_t) * num_samples);
    size_t *sample_sizes = Memory_Alloc(sizeof(size_t) * num_samples);
    File_Read(sample_offsets, sizeof(int32_t), num_samples, fp);

    const char **sample_pointers = Memory_Alloc(sizeof(char *) * num_samples);
    for (int i = 0; i < num_samples; i++) {
        sample_pointers[i] = sample_data + sample_offsets[i];
    }

    // NOTE: this assumes that sample pointers are sorted
    for (int i = 0; i < num_samples; i++) {
        int current_offset = sample_offsets[i];
        int next_offset =
            i + 1 >= num_samples ? (int)File_Size(fp) : sample_offsets[i + 1];
        sample_sizes[i] = next_offset - current_offset;
    }

    Sound_LoadSamples(num_samples, sample_pointers, sample_sizes);

    Memory_FreePointer(&sample_offsets);
    Memory_FreePointer(&sample_pointers);
    Memory_FreePointer(&sample_sizes);

    return true;
}

static bool Level_LoadTexturePages(MYFILE *fp)
{
    File_Read(&m_TexturePageCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d texture pages", m_TexturePageCount);
    uint8_t *base =
        GameBuf_Alloc(m_TexturePageCount * 256 * 256, GBUF_TEXTURE_PAGES);
    File_Read(base, 256 * 256, m_TexturePageCount, fp);
    for (int i = 0; i < m_TexturePageCount; i++) {
        g_TexturePagePtrs[i] = base;
        base += 256 * 256;
    }
    return true;
}

bool Level_Load(int level_num)
{
    LOG_INFO("%d (%s)", level_num, g_GameFlow.levels[level_num].level_file);
    bool ret =
        Level_LoadFromFile(g_GameFlow.levels[level_num].level_file, level_num);

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

    if (g_Config.disable_healing_between_levels) {
        // check if we're in main menu by seeing if there is g_Lara item in the
        // currently loaded level.
        bool lara_found = false;
        bool in_cutscene = false;
        for (int i = 0; i < g_LevelItemCount; i++) {
            if (g_Items[i].object_number == O_LARA) {
                lara_found = true;
            }
            if (g_Items[i].object_number == O_PLAYER_1
                || g_Items[i].object_number == O_PLAYER_2
                || g_Items[i].object_number == O_PLAYER_3
                || g_Items[i].object_number == O_PLAYER_4) {
                in_cutscene = true;
            }
        }
        if (!lara_found && !in_cutscene) {
            g_StoredLaraHealth = LARA_HITPOINTS;
        }
    }

    g_GameFlow.levels[level_num].secrets = GetSecretCount();
    g_GameFlow.levels[level_num].pickups = m_LevelPickups;
    g_GameFlow.levels[level_num].kills = m_LevelKillables;

    return ret;
}
