#include "specific/file.h"

#include "3dsystem/3d_gen.h"
#include "config.h"
#include "filesystem.h"
#include "game/control.h"
#include "game/items.h"
#include "game/setup.h"
#include "global/vars.h"
#include "log.h"
#include "specific/hwr.h"
#include "specific/init.h"
#include "specific/smain.h"
#include "specific/sound.h"

#include <stdio.h>
#include <windows.h>

int32_t MeshCount = 0;
int32_t MeshPtrCount = 0;
int32_t AnimCount = 0;
int32_t AnimChangeCount = 0;
int32_t AnimRangeCount = 0;
int32_t AnimCommandCount = 0;
int32_t AnimBoneCount = 0;
int32_t AnimFrameCount = 0;
int32_t ObjectCount = 0;
int32_t StaticCount = 0;
int32_t TextureCount = 0;
int32_t FloorDataSize = 0;
int32_t TexturePageCount = 0;
int32_t AnimTextureRangeCount = 0;
int32_t SpriteInfoCount = 0;
int32_t SpriteCount = 0;
int32_t OverlapCount = 0;

static bool LoadRooms(MYFILE *fp);
static bool LoadObjects(MYFILE *fp);
static bool LoadSprites(MYFILE *fp);
static bool LoadItems(MYFILE *fp);
static bool LoadDepthQ(MYFILE *fp);
static bool LoadPalette(MYFILE *fp);
static bool LoadCameras(MYFILE *fp);
static bool LoadSoundEffects(MYFILE *fp);
static bool LoadBoxes(MYFILE *fp);
static bool LoadAnimatedTextures(MYFILE *fp);
static bool LoadCinematic(MYFILE *fp);
static bool LoadDemo(MYFILE *fp);
static bool LoadSamples(MYFILE *fp);
static bool LoadTexturePages(MYFILE *fp);

bool LoadLevel(const char *filename, int32_t level_num)
{
    int32_t version;
    int32_t file_level_num;

    const char *full_path = GetFullPath(filename);
    LOG_INFO("%s", full_path);

    init_game_malloc();
    MYFILE *fp = FileOpen(full_path, FILE_OPEN_READ);
    if (!fp) {
        S_ExitSystemFmt("S_LoadLevel(): Could not open %s", full_path);
        return false;
    }

    FileRead(&version, sizeof(int32_t), 1, fp);
    if (version != 32) {
        S_ExitSystemFmt(
            "Level %d (%s) is version %d (this game code is version %d)",
            level_num, full_path, version, 32);
        return false;
    }

    if (!LoadTexturePages(fp)) {
        return false;
    }

    FileRead(&file_level_num, sizeof(int32_t), 1, fp);
    LOG_INFO("file level num: %d", file_level_num);

    if (!LoadRooms(fp)) {
        return false;
    }

    if (!LoadObjects(fp)) {
        return false;
    }

    if (!LoadSprites(fp)) {
        return false;
    }

    if (!LoadCameras(fp)) {
        return false;
    }

    if (!LoadSoundEffects(fp)) {
        return false;
    }

    if (!LoadBoxes(fp)) {
        return false;
    }

    if (!LoadAnimatedTextures(fp)) {
        return false;
    }

    if (!LoadItems(fp)) {
        return false;
    }

    if (!LoadDepthQ(fp)) {
        return false;
    }

    if (!LoadPalette(fp)) {
        return false;
    }

    if (!LoadCinematic(fp)) {
        return false;
    }

    if (!LoadDemo(fp)) {
        return false;
    }

    if (!LoadSamples(fp)) {
        return false;
    }

    FileClose(fp);

    HWR_DownloadTextures(TexturePageCount);

    return true;
}

static bool LoadRooms(MYFILE *fp)
{
    uint16_t count2;
    uint32_t count4;

    FileRead(&RoomCount, sizeof(uint16_t), 1, fp);
    LOG_INFO("%d rooms", RoomCount);

    RoomInfo = game_malloc(sizeof(ROOM_INFO) * RoomCount, GBUF_ROOM_INFOS);
    int i = 0;
    for (ROOM_INFO *current_room_info = RoomInfo; i < RoomCount;
         i++, current_room_info++) {
        // Room position
        FileRead(&current_room_info->x, sizeof(uint32_t), 1, fp);
        current_room_info->y = 0;
        FileRead(&current_room_info->z, sizeof(uint32_t), 1, fp);

        // Room floor/ceiling
        FileRead(&current_room_info->min_floor, sizeof(uint32_t), 1, fp);
        FileRead(&current_room_info->max_ceiling, sizeof(uint32_t), 1, fp);

        // Room mesh
        FileRead(&count4, sizeof(uint32_t), 1, fp);
        current_room_info->data =
            game_malloc(sizeof(uint16_t) * count4, GBUF_ROOM_MESH);
        FileRead(current_room_info->data, sizeof(uint16_t), count4, fp);

        // Doors
        FileRead(&count2, sizeof(uint16_t), 1, fp);
        if (!count2) {
            current_room_info->doors = NULL;
        } else {
            current_room_info->doors = game_malloc(
                sizeof(uint16_t) + sizeof(DOOR_INFO) * count2, GBUF_ROOM_DOOR);
            current_room_info->doors->count = count2;
            FileRead(
                &current_room_info->doors->door, sizeof(DOOR_INFO), count2, fp);
        }

        // Room floor
        FileRead(&current_room_info->x_size, sizeof(uint16_t), 1, fp);
        FileRead(&current_room_info->y_size, sizeof(uint16_t), 1, fp);
        count4 = current_room_info->y_size * current_room_info->x_size;
        current_room_info->floor =
            game_malloc(sizeof(FLOOR_INFO) * count4, GBUF_ROOM_FLOOR);
        FileRead(current_room_info->floor, sizeof(FLOOR_INFO), count4, fp);

        // Room lights
        FileRead(&current_room_info->ambient, sizeof(uint16_t), 1, fp);
        FileRead(&current_room_info->num_lights, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_lights) {
            current_room_info->light = NULL;
        } else {
            current_room_info->light = game_malloc(
                sizeof(LIGHT_INFO) * current_room_info->num_lights,
                GBUF_ROOM_LIGHTS);
            FileRead(
                current_room_info->light, sizeof(LIGHT_INFO),
                current_room_info->num_lights, fp);
        }

        // Static mesh infos
        FileRead(&current_room_info->num_meshes, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_meshes) {
            current_room_info->mesh = NULL;
        } else {
            current_room_info->mesh = game_malloc(
                sizeof(MESH_INFO) * current_room_info->num_meshes,
                GBUF_ROOM_STATIC_MESH_INFOS);
            FileRead(
                current_room_info->mesh, sizeof(MESH_INFO),
                current_room_info->num_meshes, fp);
        }

        // Flipped (alternative) room
        FileRead(&current_room_info->flipped_room, sizeof(uint16_t), 1, fp);

        // Room flags
        FileRead(&current_room_info->flags, sizeof(uint16_t), 1, fp);

        // Initialise some variables
        current_room_info->bound_active = 0;
        current_room_info->left = PhdWinMaxX;
        current_room_info->top = PhdWinMaxY;
        current_room_info->bottom = 0;
        current_room_info->right = 0;
        current_room_info->item_number = -1;
        current_room_info->fx_number = -1;
    }

    FileRead(&FloorDataSize, sizeof(uint32_t), 1, fp);
    FloorData = game_malloc(sizeof(uint16_t) * FloorDataSize, GBUF_FLOOR_DATA);
    FileRead(FloorData, sizeof(uint16_t), FloorDataSize, fp);

    return true;
}

static bool LoadObjects(MYFILE *fp)
{
    FileRead(&MeshCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d meshes", MeshCount);
    MeshBase = game_malloc(sizeof(int16_t) * MeshCount, GBUF_MESHES);
    FileRead(MeshBase, sizeof(int16_t), MeshCount, fp);

    FileRead(&MeshPtrCount, sizeof(int32_t), 1, fp);
    uint32_t *mesh_indices =
        game_malloc(sizeof(uint32_t) * MeshPtrCount, GBUF_MESH_POINTERS);
    FileRead(mesh_indices, sizeof(uint32_t), MeshPtrCount, fp);

    Meshes = game_malloc(sizeof(int16_t *) * MeshPtrCount, GBUF_MESH_POINTERS);
    for (int i = 0; i < MeshPtrCount; i++) {
        Meshes[i] = &MeshBase[mesh_indices[i] / 2];
    }

    FileRead(&AnimCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anims", AnimCount);
    Anims = game_malloc(sizeof(ANIM_STRUCT) * AnimCount, GBUF_ANIMS);
    FileRead(Anims, sizeof(ANIM_STRUCT), AnimCount, fp);

    FileRead(&AnimChangeCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim changes", AnimChangeCount);
    AnimChanges = game_malloc(
        sizeof(ANIM_CHANGE_STRUCT) * AnimChangeCount, GBUF_ANIM_CHANGES);
    FileRead(AnimChanges, sizeof(ANIM_CHANGE_STRUCT), AnimChangeCount, fp);

    FileRead(&AnimRangeCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim ranges", AnimRangeCount);
    AnimRanges = game_malloc(
        sizeof(ANIM_RANGE_STRUCT) * AnimRangeCount, GBUF_ANIM_RANGES);
    FileRead(AnimRanges, sizeof(ANIM_RANGE_STRUCT), AnimRangeCount, fp);

    FileRead(&AnimCommandCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim commands", AnimCommandCount);
    AnimCommands =
        game_malloc(sizeof(int16_t) * AnimCommandCount, GBUF_ANIM_COMMANDS);
    FileRead(AnimCommands, sizeof(int16_t), AnimCommandCount, fp);

    FileRead(&AnimBoneCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim bones", AnimBoneCount);
    AnimBones = game_malloc(sizeof(int32_t) * AnimBoneCount, GBUF_ANIM_BONES);
    FileRead(AnimBones, sizeof(int32_t), AnimBoneCount, fp);

    FileRead(&AnimFrameCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim frames", AnimFrameCount);
    AnimFrames =
        game_malloc(sizeof(int16_t) * AnimFrameCount, GBUF_ANIM_FRAMES);
    FileRead(AnimFrames, sizeof(int16_t), AnimFrameCount, fp);
    for (int i = 0; i < AnimCount; i++) {
        Anims[i].frame_ptr = &AnimFrames[(size_t)Anims[i].frame_ptr / 2];
    }

    FileRead(&ObjectCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d objects", ObjectCount);
    for (int i = 0; i < ObjectCount; i++) {
        int32_t tmp;
        FileRead(&tmp, sizeof(int32_t), 1, fp);
        OBJECT_INFO *object = &Objects[tmp];

        FileRead(&object->nmeshes, sizeof(int16_t), 1, fp);
        FileRead(&object->mesh_index, sizeof(int16_t), 1, fp);
        FileRead(&object->bone_index, sizeof(int32_t), 1, fp);

        FileRead(&tmp, sizeof(int32_t), 1, fp);
        object->frame_base = &AnimFrames[tmp / 2];
        FileRead(&object->anim_index, sizeof(int16_t), 1, fp);
        object->loaded = 1;
    }

    InitialiseObjects();

    FileRead(&StaticCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d statics", StaticCount);
    for (int i = 0; i < StaticCount; i++) {
        int32_t tmp;
        FileRead(&tmp, sizeof(int32_t), 1, fp);
        STATIC_INFO *object = &StaticObjects[tmp];

        FileRead(&object->mesh_number, sizeof(int16_t), 1, fp);
        FileRead(&object->x_minp, sizeof(int16_t), 6, fp);
        FileRead(&object->x_minc, sizeof(int16_t), 6, fp);
        FileRead(&object->flags, sizeof(int16_t), 1, fp);
    }

    FileRead(&TextureCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d textures", TextureCount);
    if (TextureCount > MAX_TEXTURES) {
        S_ExitSystem("Too many Textures in level");
        return false;
    }
    FileRead(PhdTextureInfo, sizeof(PHD_TEXTURE), TextureCount, fp);

    return true;
}

static bool LoadSprites(MYFILE *fp)
{
    FileRead(&SpriteInfoCount, sizeof(int32_t), 1, fp);
    if (SpriteInfoCount > MAX_SPRITES) {
        S_ExitSystem("Too many sprites in level");
        return false;
    }
    FileRead(&PhdSpriteInfo, sizeof(PHD_SPRITE), SpriteInfoCount, fp);

    FileRead(&SpriteCount, sizeof(int32_t), 1, fp);
    for (int i = 0; i < SpriteCount; i++) {
        int32_t object_num;
        FileRead(&object_num, sizeof(int32_t), 1, fp);
        if (object_num < O_NUMBER_OF) {
            FileRead(&Objects[object_num], sizeof(int16_t), 1, fp);
            FileRead(&Objects[object_num].mesh_index, sizeof(int16_t), 1, fp);
            Objects[object_num].loaded = 1;
        } else {
            int32_t static_num = object_num - O_NUMBER_OF;
            FileSeek(fp, 2, FILE_SEEK_CUR);
            FileRead(
                &StaticObjects[static_num].mesh_number, sizeof(int16_t), 1, fp);
        }
    }
    return true;
}

static bool LoadItems(MYFILE *fp)
{
    int32_t item_count = 0;
    FileRead(&item_count, sizeof(int32_t), 1, fp);

    LOG_INFO("%d items", item_count);

    if (item_count) {
        if (item_count > MAX_ITEMS) {
            S_ExitSystem("LoadItems(): Too Many Items being Loaded!!");
            return false;
        }

        Items = game_malloc(sizeof(ITEM_INFO) * MAX_ITEMS, GBUF_ITEMS);
        LevelItemCount = item_count;
        InitialiseItemArray(MAX_ITEMS);

        for (int i = 0; i < item_count; i++) {
            ITEM_INFO *item = &Items[i];
            FileRead(&item->object_number, sizeof(int16_t), 1, fp);
            FileRead(&item->room_number, sizeof(int16_t), 1, fp);
            FileRead(&item->pos.x, sizeof(int32_t), 1, fp);
            FileRead(&item->pos.y, sizeof(int32_t), 1, fp);
            FileRead(&item->pos.z, sizeof(int32_t), 1, fp);
            FileRead(&item->pos.y_rot, sizeof(int16_t), 1, fp);
            FileRead(&item->shade, sizeof(int16_t), 1, fp);
            FileRead(&item->flags, sizeof(uint16_t), 1, fp);

            if (item->object_number < 0 || item->object_number >= O_NUMBER_OF) {
                S_ExitSystemFmt(
                    "LoadItems(): Bad Object number (%d) on Item %d",
                    item->object_number, i);
            }

            InitialiseItem(i);
        }
    }

    return true;
}

static bool LoadDepthQ(MYFILE *fp)
{
    LOG_INFO("");
    FileSeek(fp, sizeof(uint8_t) * 32 * 256, FILE_SEEK_CUR);
    return true;
}

static bool LoadPalette(MYFILE *fp)
{
    LOG_INFO("");
    FileRead(GamePalette, sizeof(uint8_t), 256 * 3, fp);
    GamePalette[0].r = 0;
    GamePalette[0].g = 0;
    GamePalette[0].b = 0;
    HWR_SetPalette();
    return true;
}

static bool LoadCameras(MYFILE *fp)
{
    FileRead(&NumberCameras, sizeof(int32_t), 1, fp);
    LOG_INFO("%d cameras", NumberCameras);
    if (!NumberCameras) {
        return true;
    }
    Camera.fixed =
        game_malloc(sizeof(OBJECT_VECTOR) * NumberCameras, GBUF_CAMERAS);
    if (!Camera.fixed) {
        return false;
    }
    FileRead(Camera.fixed, sizeof(OBJECT_VECTOR), NumberCameras, fp);
    return true;
}

static bool LoadSoundEffects(MYFILE *fp)
{
    FileRead(&NumberSoundEffects, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sound effects", NumberSoundEffects);
    if (!NumberSoundEffects) {
        return true;
    }
    SoundEffectsTable =
        game_malloc(sizeof(OBJECT_VECTOR) * NumberSoundEffects, GBUF_SOUND_FX);
    if (!SoundEffectsTable) {
        return false;
    }
    FileRead(SoundEffectsTable, sizeof(OBJECT_VECTOR), NumberSoundEffects, fp);
    return true;
}

static bool LoadBoxes(MYFILE *fp)
{
    FileRead(&NumberBoxes, sizeof(int32_t), 1, fp);
    Boxes = game_malloc(sizeof(BOX_INFO) * NumberBoxes, GBUF_BOXES);
    if (!FileRead(Boxes, sizeof(BOX_INFO), NumberBoxes, fp)) {
        S_ExitSystem("LoadBoxes(): Unable to load boxes");
        return false;
    }

    FileRead(&OverlapCount, sizeof(int32_t), 1, fp);
    Overlap = (uint16_t *)game_malloc(sizeof(uint16_t) * OverlapCount, 22);
    if (!FileRead(Overlap, sizeof(uint16_t), OverlapCount, fp)) {
        S_ExitSystem("LoadBoxes(): Unable to load box overlaps");
        return false;
    }

    for (int i = 0; i < 2; i++) {
        GroundZone[i] =
            game_malloc(sizeof(int16_t) * NumberBoxes, GBUF_GROUNDZONE);
        if (!GroundZone[i]
            || !FileRead(GroundZone[i], sizeof(int16_t), NumberBoxes, fp)) {
            S_ExitSystem("LoadBoxes(): Unable to load 'ground_zone'");
            return false;
        }

        GroundZone2[i] =
            game_malloc(sizeof(int16_t) * NumberBoxes, GBUF_GROUNDZONE);
        if (!GroundZone2[i]
            || !FileRead(GroundZone2[i], sizeof(int16_t), NumberBoxes, fp)) {
            S_ExitSystem("LoadBoxes(): Unable to load 'ground2_zone'");
            return false;
        }

        FlyZone[i] = game_malloc(sizeof(int16_t) * NumberBoxes, GBUF_FLYZONE);
        if (!FlyZone[i]
            || !FileRead(FlyZone[i], sizeof(int16_t), NumberBoxes, fp)) {
            S_ExitSystem("LoadBoxes(): Unable to load 'fly_zone'");
            return false;
        }
    }

    return true;
}

static bool LoadAnimatedTextures(MYFILE *fp)
{
    FileRead(&AnimTextureRangeCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d animated textures", AnimTextureRangeCount);
    AnimTextureRanges = game_malloc(
        sizeof(int16_t) * AnimTextureRangeCount, GBUF_ANIMATING_TEXTURE_RANGES);
    FileRead(AnimTextureRanges, sizeof(int16_t), AnimTextureRangeCount, fp);
    return true;
}

static bool LoadCinematic(MYFILE *fp)
{
    FileRead(&NumCineFrames, sizeof(int16_t), 1, fp);
    LOG_INFO("%d cinematic frames", NumCineFrames);
    if (!NumCineFrames) {
        return true;
    }
    Cine =
        game_malloc(sizeof(int16_t) * 8 * NumCineFrames, GBUF_CINEMATIC_FRAMES);
    FileRead(Cine, sizeof(int16_t) * 8, NumCineFrames, fp);
    return true;
}

static bool LoadDemo(MYFILE *fp)
{
    DemoData =
        game_malloc(sizeof(uint32_t) * DEMO_COUNT_MAX, GBUF_LOADDEMO_BUFFER);
    uint16_t size = 0;
    FileRead(&size, sizeof(int16_t), 1, fp);
    LOG_INFO("%d demo buffer size", size);
    if (!size) {
        return true;
    }
    FileRead(DemoData, 1, size, fp);
    return true;
}

static bool LoadSamples(MYFILE *fp)
{
    if (!SoundIsActive) {
        return true;
    }

    FileRead(SampleLUT, sizeof(int16_t), MAX_SAMPLES, fp);
    int32_t num_sample_infos;
    FileRead(&num_sample_infos, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sample infos", num_sample_infos);
    if (!num_sample_infos) {
        S_ExitSystem("No Sample Infos");
        return false;
    }

    SampleInfos =
        game_malloc(sizeof(SAMPLE_INFO) * num_sample_infos, GBUF_SAMPLE_INFOS);
    FileRead(SampleInfos, sizeof(SAMPLE_INFO), num_sample_infos, fp);

    int32_t sample_data_size;
    FileRead(&sample_data_size, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sample data size", sample_data_size);
    if (!sample_data_size) {
        S_ExitSystem("No Sample Data");
        return false;
    }

    char *sample_data = game_malloc(sample_data_size, GBUF_SAMPLES);
    FileRead(sample_data, sizeof(char), sample_data_size, fp);

    int32_t num_samples;
    FileRead(&num_samples, sizeof(int32_t), 1, fp);
    LOG_INFO("%d samples", num_samples);
    if (!num_samples) {
        S_ExitSystem("No Samples");
        return false;
    }

    int32_t *sample_offsets =
        game_malloc(sizeof(int32_t) * num_samples, GBUF_SAMPLE_OFFSETS);
    FileRead(sample_offsets, sizeof(int32_t), num_samples, fp);

    char **sample_pointers =
        game_malloc(sizeof(char *) * num_samples, GBUF_SAMPLE_OFFSETS);
    for (int i = 0; i < num_samples; i++) {
        sample_pointers[i] = sample_data + sample_offsets[i];
    }

    S_Sound_LoadSamples(sample_pointers, num_samples);

    game_free(sizeof(char *) * num_samples, GBUF_SAMPLE_OFFSETS);

    return true;
}

static bool LoadTexturePages(MYFILE *fp)
{
    FileRead(&TexturePageCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d texture pages", TexturePageCount);
    int8_t *base = game_malloc(TexturePageCount * 65536, GBUF_TEXTURE_PAGES);
    FileRead(base, 65536, TexturePageCount, fp);
    for (int i = 0; i < TexturePageCount; i++) {
        TexturePagePtrs[i] = base;
        base += 65536;
    }
    return true;
}

bool S_LoadLevel(int level_num)
{
    LOG_INFO("%d (%s)", level_num, GF.levels[level_num].level_file);
    bool ret = LoadLevel(GF.levels[level_num].level_file, level_num);

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

const char *GetFullPath(const char *filename)
{
    static char newpath[128];
    LOG_INFO("%s", filename);
    sprintf(newpath, ".\\%s", filename);
    return newpath;
}

void FileLoad(const char *path, char **output_data, size_t *output_size)
{
    MYFILE *fp = FileOpen(path, FILE_OPEN_READ);
    if (!fp) {
        ShowFatalError("File load error");
        return;
    }

    size_t data_size = FileSize(fp);
    char *data = malloc(data_size);
    if (!data) {
        ShowFatalError("Failed to allocate memory");
        return;
    }
    if (FileRead(data, sizeof(char), data_size, fp) != data_size) {
        ShowFatalError("File read error");
        return;
    }
    FileClose(fp);

    *output_data = data;
    *output_size = data_size;
}
