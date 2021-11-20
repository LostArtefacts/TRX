#include "game/level.h"

#include "3dsystem/3d_gen.h"
#include "config.h"
#include "filesystem.h"
#include "game/control.h"
#include "game/gamebuf.h"
#include "game/items.h"
#include "game/setup.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "global/vars.h"
#include "log.h"
#include "memory.h"
#include "specific/s_hwr.h"
#include "specific/s_shell.h"

#include <stdio.h>

int32_t m_MeshCount = 0;
int32_t m_MeshPtrCount = 0;
int32_t m_AnimCount = 0;
int32_t m_AnimChangeCount = 0;
int32_t m_AnimRangeCount = 0;
int32_t m_AnimCommandCount = 0;
int32_t m_AnimBoneCount = 0;
int32_t m_AnimFrameCount = 0;
int32_t m_ObjectCount = 0;
int32_t m_StaticCount = 0;
int32_t m_TextureCount = 0;
int32_t m_FloorDataSize = 0;
int32_t m_TexturePageCount = 0;
int32_t m_AnimTextureRangeCount = 0;
int32_t m_SpriteInfoCount = 0;
int32_t m_SpriteCount = 0;
int32_t m_OverlapCount = 0;

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

    GameBuf_Init();
    MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
    if (!fp) {
        S_Shell_ExitSystemFmt(
            "Level_LoadFromFile(): Could not open %s", filename);
        return false;
    }

    File_Read(&version, sizeof(int32_t), 1, fp);
    if (version != 32) {
        S_Shell_ExitSystemFmt(
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

    HWR_DownloadTextures(m_TexturePageCount);

    return true;
}

static bool Level_LoadRooms(MYFILE *fp)
{
    uint16_t count2;
    uint32_t count4;

    File_Read(&RoomCount, sizeof(uint16_t), 1, fp);
    LOG_INFO("%d rooms", RoomCount);

    RoomInfo = GameBuf_Alloc(sizeof(ROOM_INFO) * RoomCount, GBUF_ROOM_INFOS);
    int i = 0;
    for (ROOM_INFO *current_room_info = RoomInfo; i < RoomCount;
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
    FloorData =
        GameBuf_Alloc(sizeof(uint16_t) * m_FloorDataSize, GBUF_FLOOR_DATA);
    File_Read(FloorData, sizeof(uint16_t), m_FloorDataSize, fp);

    return true;
}

static bool Level_LoadObjects(MYFILE *fp)
{
    File_Read(&m_MeshCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d meshes", m_MeshCount);
    MeshBase = GameBuf_Alloc(sizeof(int16_t) * m_MeshCount, GBUF_MESHES);
    File_Read(MeshBase, sizeof(int16_t), m_MeshCount, fp);

    File_Read(&m_MeshPtrCount, sizeof(int32_t), 1, fp);
    uint32_t *mesh_indices =
        GameBuf_Alloc(sizeof(uint32_t) * m_MeshPtrCount, GBUF_MESH_POINTERS);
    File_Read(mesh_indices, sizeof(uint32_t), m_MeshPtrCount, fp);

    Meshes =
        GameBuf_Alloc(sizeof(int16_t *) * m_MeshPtrCount, GBUF_MESH_POINTERS);
    for (int i = 0; i < m_MeshPtrCount; i++) {
        Meshes[i] = &MeshBase[mesh_indices[i] / 2];
    }

    File_Read(&m_AnimCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anims", m_AnimCount);
    Anims = GameBuf_Alloc(sizeof(ANIM_STRUCT) * m_AnimCount, GBUF_ANIMS);
    File_Read(Anims, sizeof(ANIM_STRUCT), m_AnimCount, fp);

    File_Read(&m_AnimChangeCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim changes", m_AnimChangeCount);
    AnimChanges = GameBuf_Alloc(
        sizeof(ANIM_CHANGE_STRUCT) * m_AnimChangeCount, GBUF_ANIM_CHANGES);
    File_Read(AnimChanges, sizeof(ANIM_CHANGE_STRUCT), m_AnimChangeCount, fp);

    File_Read(&m_AnimRangeCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim ranges", m_AnimRangeCount);
    AnimRanges = GameBuf_Alloc(
        sizeof(ANIM_RANGE_STRUCT) * m_AnimRangeCount, GBUF_ANIM_RANGES);
    File_Read(AnimRanges, sizeof(ANIM_RANGE_STRUCT), m_AnimRangeCount, fp);

    File_Read(&m_AnimCommandCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim commands", m_AnimCommandCount);
    AnimCommands =
        GameBuf_Alloc(sizeof(int16_t) * m_AnimCommandCount, GBUF_ANIM_COMMANDS);
    File_Read(AnimCommands, sizeof(int16_t), m_AnimCommandCount, fp);

    File_Read(&m_AnimBoneCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim bones", m_AnimBoneCount);
    AnimBones =
        GameBuf_Alloc(sizeof(int32_t) * m_AnimBoneCount, GBUF_ANIM_BONES);
    File_Read(AnimBones, sizeof(int32_t), m_AnimBoneCount, fp);

    File_Read(&m_AnimFrameCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim frames", m_AnimFrameCount);
    AnimFrames =
        GameBuf_Alloc(sizeof(int16_t) * m_AnimFrameCount, GBUF_ANIM_FRAMES);
    File_Read(AnimFrames, sizeof(int16_t), m_AnimFrameCount, fp);
    for (int i = 0; i < m_AnimCount; i++) {
        Anims[i].frame_ptr = &AnimFrames[(size_t)Anims[i].frame_ptr / 2];
    }

    File_Read(&m_ObjectCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d objects", m_ObjectCount);
    for (int i = 0; i < m_ObjectCount; i++) {
        int32_t tmp;
        File_Read(&tmp, sizeof(int32_t), 1, fp);
        OBJECT_INFO *object = &Objects[tmp];

        File_Read(&object->nmeshes, sizeof(int16_t), 1, fp);
        File_Read(&object->mesh_index, sizeof(int16_t), 1, fp);
        File_Read(&object->bone_index, sizeof(int32_t), 1, fp);

        File_Read(&tmp, sizeof(int32_t), 1, fp);
        object->frame_base = &AnimFrames[tmp / 2];
        File_Read(&object->anim_index, sizeof(int16_t), 1, fp);
        object->loaded = 1;
    }

    InitialiseObjects();

    File_Read(&m_StaticCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d statics", m_StaticCount);
    for (int i = 0; i < m_StaticCount; i++) {
        int32_t tmp;
        File_Read(&tmp, sizeof(int32_t), 1, fp);
        STATIC_INFO *object = &StaticObjects[tmp];

        File_Read(&object->mesh_number, sizeof(int16_t), 1, fp);
        File_Read(&object->x_minp, sizeof(int16_t), 6, fp);
        File_Read(&object->x_minc, sizeof(int16_t), 6, fp);
        File_Read(&object->flags, sizeof(int16_t), 1, fp);
    }

    File_Read(&m_TextureCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d textures", m_TextureCount);
    if (m_TextureCount > MAX_TEXTURES) {
        S_Shell_ExitSystem("Too many Textures in level");
        return false;
    }
    File_Read(PhdTextureInfo, sizeof(PHD_TEXTURE), m_TextureCount, fp);

    return true;
}

static bool Level_LoadSprites(MYFILE *fp)
{
    File_Read(&m_SpriteInfoCount, sizeof(int32_t), 1, fp);
    if (m_SpriteInfoCount > MAX_SPRITES) {
        S_Shell_ExitSystem("Too many sprites in level");
        return false;
    }
    File_Read(&PhdSpriteInfo, sizeof(PHD_SPRITE), m_SpriteInfoCount, fp);

    File_Read(&m_SpriteCount, sizeof(int32_t), 1, fp);
    for (int i = 0; i < m_SpriteCount; i++) {
        int32_t object_num;
        File_Read(&object_num, sizeof(int32_t), 1, fp);
        if (object_num < O_NUMBER_OF) {
            File_Read(&Objects[object_num], sizeof(int16_t), 1, fp);
            File_Read(&Objects[object_num].mesh_index, sizeof(int16_t), 1, fp);
            Objects[object_num].loaded = 1;
        } else {
            int32_t static_num = object_num - O_NUMBER_OF;
            File_Seek(fp, 2, FILE_SEEK_CUR);
            File_Read(
                &StaticObjects[static_num].mesh_number, sizeof(int16_t), 1, fp);
        }
    }
    return true;
}

static bool Level_LoadItems(MYFILE *fp)
{
    int32_t item_count = 0;
    File_Read(&item_count, sizeof(int32_t), 1, fp);

    LOG_INFO("%d items", item_count);

    if (item_count) {
        if (item_count > MAX_ITEMS) {
            S_Shell_ExitSystem(
                "Level_LoadItems(): Too Many Items being Loaded!!");
            return false;
        }

        Items = GameBuf_Alloc(sizeof(ITEM_INFO) * MAX_ITEMS, GBUF_ITEMS);
        LevelItemCount = item_count;
        InitialiseItemArray(MAX_ITEMS);

        for (int i = 0; i < item_count; i++) {
            ITEM_INFO *item = &Items[i];
            File_Read(&item->object_number, sizeof(int16_t), 1, fp);
            File_Read(&item->room_number, sizeof(int16_t), 1, fp);
            File_Read(&item->pos.x, sizeof(int32_t), 1, fp);
            File_Read(&item->pos.y, sizeof(int32_t), 1, fp);
            File_Read(&item->pos.z, sizeof(int32_t), 1, fp);
            File_Read(&item->pos.y_rot, sizeof(int16_t), 1, fp);
            File_Read(&item->shade, sizeof(int16_t), 1, fp);
            File_Read(&item->flags, sizeof(uint16_t), 1, fp);

            if (item->object_number < 0 || item->object_number >= O_NUMBER_OF) {
                S_Shell_ExitSystemFmt(
                    "Level_LoadItems(): Bad Object number (%d) on Item %d",
                    item->object_number, i);
            }

            InitialiseItem(i);
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
    File_Read(GamePalette, sizeof(uint8_t), 256 * 3, fp);
    GamePalette[0].r = 0;
    GamePalette[0].g = 0;
    GamePalette[0].b = 0;
    HWR_SetPalette();
    return true;
}

static bool Level_LoadCameras(MYFILE *fp)
{
    File_Read(&NumberCameras, sizeof(int32_t), 1, fp);
    LOG_INFO("%d cameras", NumberCameras);
    if (!NumberCameras) {
        return true;
    }
    Camera.fixed =
        GameBuf_Alloc(sizeof(OBJECT_VECTOR) * NumberCameras, GBUF_CAMERAS);
    if (!Camera.fixed) {
        return false;
    }
    File_Read(Camera.fixed, sizeof(OBJECT_VECTOR), NumberCameras, fp);
    return true;
}

static bool Level_LoadSoundEffects(MYFILE *fp)
{
    File_Read(&NumberSoundEffects, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sound effects", NumberSoundEffects);
    if (!NumberSoundEffects) {
        return true;
    }
    SoundEffectsTable = GameBuf_Alloc(
        sizeof(OBJECT_VECTOR) * NumberSoundEffects, GBUF_SOUND_FX);
    if (!SoundEffectsTable) {
        return false;
    }
    File_Read(SoundEffectsTable, sizeof(OBJECT_VECTOR), NumberSoundEffects, fp);
    return true;
}

static bool Level_LoadBoxes(MYFILE *fp)
{
    File_Read(&NumberBoxes, sizeof(int32_t), 1, fp);
    Boxes = GameBuf_Alloc(sizeof(BOX_INFO) * NumberBoxes, GBUF_BOXES);
    if (!File_Read(Boxes, sizeof(BOX_INFO), NumberBoxes, fp)) {
        S_Shell_ExitSystem("Level_LoadBoxes(): Unable to load boxes");
        return false;
    }

    File_Read(&m_OverlapCount, sizeof(int32_t), 1, fp);
    Overlap = GameBuf_Alloc(sizeof(uint16_t) * m_OverlapCount, 22);
    if (!File_Read(Overlap, sizeof(uint16_t), m_OverlapCount, fp)) {
        S_Shell_ExitSystem("Level_LoadBoxes(): Unable to load box overlaps");
        return false;
    }

    for (int i = 0; i < 2; i++) {
        GroundZone[i] =
            GameBuf_Alloc(sizeof(int16_t) * NumberBoxes, GBUF_GROUNDZONE);
        if (!GroundZone[i]
            || !File_Read(GroundZone[i], sizeof(int16_t), NumberBoxes, fp)) {
            S_Shell_ExitSystem(
                "Level_LoadBoxes(): Unable to load 'ground_zone'");
            return false;
        }

        GroundZone2[i] =
            GameBuf_Alloc(sizeof(int16_t) * NumberBoxes, GBUF_GROUNDZONE);
        if (!GroundZone2[i]
            || !File_Read(GroundZone2[i], sizeof(int16_t), NumberBoxes, fp)) {
            S_Shell_ExitSystem(
                "Level_LoadBoxes(): Unable to load 'ground2_zone'");
            return false;
        }

        FlyZone[i] = GameBuf_Alloc(sizeof(int16_t) * NumberBoxes, GBUF_FLYZONE);
        if (!FlyZone[i]
            || !File_Read(FlyZone[i], sizeof(int16_t), NumberBoxes, fp)) {
            S_Shell_ExitSystem("Level_LoadBoxes(): Unable to load 'fly_zone'");
            return false;
        }
    }

    return true;
}

static bool Level_LoadAnimatedTextures(MYFILE *fp)
{
    File_Read(&m_AnimTextureRangeCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d animated textures", m_AnimTextureRangeCount);
    AnimTextureRanges = GameBuf_Alloc(
        sizeof(int16_t) * m_AnimTextureRangeCount,
        GBUF_ANIMATING_TEXTURE_RANGES);
    File_Read(AnimTextureRanges, sizeof(int16_t), m_AnimTextureRangeCount, fp);
    return true;
}

static bool Level_LoadCinematic(MYFILE *fp)
{
    File_Read(&NumCineFrames, sizeof(int16_t), 1, fp);
    LOG_INFO("%d cinematic frames", NumCineFrames);
    if (!NumCineFrames) {
        return true;
    }
    Cine = GameBuf_Alloc(
        sizeof(int16_t) * 8 * NumCineFrames, GBUF_CINEMATIC_FRAMES);
    File_Read(Cine, sizeof(int16_t) * 8, NumCineFrames, fp);
    return true;
}

static bool Level_LoadDemo(MYFILE *fp)
{
    DemoData =
        GameBuf_Alloc(sizeof(uint32_t) * DEMO_COUNT_MAX, GBUF_LOADDEMO_BUFFER);
    uint16_t size = 0;
    File_Read(&size, sizeof(int16_t), 1, fp);
    LOG_INFO("%d demo buffer size", size);
    if (!size) {
        return true;
    }
    File_Read(DemoData, 1, size, fp);
    return true;
}

static bool Level_LoadSamples(MYFILE *fp)
{
    if (!SoundIsActive) {
        return true;
    }

    File_Read(SampleLUT, sizeof(int16_t), MAX_SAMPLES, fp);
    int32_t num_sample_infos;
    File_Read(&num_sample_infos, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sample infos", num_sample_infos);
    if (!num_sample_infos) {
        S_Shell_ExitSystem("No Sample Infos");
        return false;
    }

    SampleInfos = GameBuf_Alloc(
        sizeof(SAMPLE_INFO) * num_sample_infos, GBUF_SAMPLE_INFOS);
    File_Read(SampleInfos, sizeof(SAMPLE_INFO), num_sample_infos, fp);

    int32_t sample_data_size;
    File_Read(&sample_data_size, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sample data size", sample_data_size);
    if (!sample_data_size) {
        S_Shell_ExitSystem("No Sample Data");
        return false;
    }

    char *sample_data = GameBuf_Alloc(sample_data_size, GBUF_SAMPLES);
    File_Read(sample_data, sizeof(char), sample_data_size, fp);

    int32_t num_samples;
    File_Read(&num_samples, sizeof(int32_t), 1, fp);
    LOG_INFO("%d samples", num_samples);
    if (!num_samples) {
        S_Shell_ExitSystem("No Samples");
        return false;
    }

    int32_t *sample_offsets = Memory_Alloc(sizeof(int32_t) * num_samples);
    File_Read(sample_offsets, sizeof(int32_t), num_samples, fp);

    char **sample_pointers = Memory_Alloc(sizeof(char *) * num_samples);
    for (int i = 0; i < num_samples; i++) {
        sample_pointers[i] = sample_data + sample_offsets[i];
    }

    Sound_LoadSamples(sample_pointers, num_samples);

    Memory_Free(sample_offsets);
    Memory_Free(sample_pointers);

    return true;
}

static bool Level_LoadTexturePages(MYFILE *fp)
{
    File_Read(&m_TexturePageCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d texture pages", m_TexturePageCount);
    int8_t *base =
        GameBuf_Alloc(m_TexturePageCount * 65536, GBUF_TEXTURE_PAGES);
    File_Read(base, 65536, m_TexturePageCount, fp);
    for (int i = 0; i < m_TexturePageCount; i++) {
        TexturePagePtrs[i] = base;
        base += 65536;
    }
    return true;
}

bool Level_Load(int level_num)
{
    LOG_INFO("%d (%s)", level_num, GF.levels[level_num].level_file);
    bool ret = Level_LoadFromFile(GF.levels[level_num].level_file, level_num);

    HWR_SetWaterColor(
        GF.levels[level_num].water_color.override
            ? &GF.levels[level_num].water_color.value
            : &GF.water_color);

    phd_SetDrawDistFade(
        (GF.levels[level_num].draw_distance_fade.override
             ? GF.levels[level_num].draw_distance_fade.value
             : GF.draw_distance_fade)
        * WALL_L);

    phd_SetDrawDistMax(
        (GF.levels[level_num].draw_distance_max.override
             ? GF.levels[level_num].draw_distance_max.value
             : GF.draw_distance_max)
        * WALL_L);

    if (T1MConfig.disable_healing_between_levels) {
        // check if we're in main menu by seeing if there is Lara item in the
        // currently loaded level.
        bool lara_found = false;
        bool in_cutscene = false;
        for (int i = 0; i < LevelItemCount; i++) {
            if (Items[i].object_number == O_LARA) {
                lara_found = true;
            }
            if (Items[i].object_number == O_PLAYER_1
                || Items[i].object_number == O_PLAYER_2
                || Items[i].object_number == O_PLAYER_3
                || Items[i].object_number == O_PLAYER_4) {
                in_cutscene = true;
            }
        }
        if (!lara_found && !in_cutscene) {
            StoredLaraHealth = LARA_HITPOINTS;
        }
    }

    GF.levels[level_num].secrets = GetSecretCount();

    return ret;
}
