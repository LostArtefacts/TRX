#include "specific/file.h"

#include "config.h"
#include "filesystem.h"
#include "game/control.h"
#include "game/items.h"
#include "game/setup.h"
#include "global/vars.h"
#include "specific/hwr.h"
#include "specific/init.h"
#include "specific/smain.h"
#include "specific/sndpc.h"
#include "util.h"

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

static int32_t LoadRooms(MYFILE *fp);
static int32_t LoadObjects(MYFILE *fp);
static int32_t LoadSprites(MYFILE *fp);
static int32_t LoadItems(MYFILE *fp);
static int32_t LoadDepthQ(MYFILE *fp);
static int32_t LoadPalette(MYFILE *fp);
static int32_t LoadCameras(MYFILE *fp);
static int32_t LoadSoundEffects(MYFILE *fp);
static int32_t LoadBoxes(MYFILE *fp);
static int32_t LoadAnimatedTextures(MYFILE *fp);
static int32_t LoadCinematic(MYFILE *fp);
static int32_t LoadDemo(MYFILE *fp);
static int32_t LoadSamples(MYFILE *fp);
static int32_t LoadTexturePages(MYFILE *fp);

int32_t LoadLevel(const char *filename, int32_t level_num)
{
    int32_t version;
    int32_t file_level_num;

    const char *full_path = GetFullPath(filename);
    LOG_INFO("%s", full_path);

    init_game_malloc();
    MYFILE *fp = FileOpen(full_path, FILE_OPEN_READ);
    if (!fp) {
        sprintf(StringToShow, "S_LoadLevel(): Could not open %s", full_path);
        S_ExitSystem(StringToShow);
        return 0;
    }

    FileRead(&version, sizeof(int32_t), 1, fp);
    if (version != 32) {
        sprintf(
            StringToShow,
            "Level %d (%s) is version %d (this game code is version %d)",
            level_num, full_path, version, 32);
        S_ExitSystem(StringToShow);
        return 0;
    }

    if (!LoadTexturePages(fp)) {
        return 0;
    }

    FileRead(&file_level_num, sizeof(int32_t), 1, fp);
    LOG_INFO("file level num: %d", file_level_num);

    if (!LoadRooms(fp)) {
        return 0;
    }

    if (!LoadObjects(fp)) {
        return 0;
    }

    if (!LoadSprites(fp)) {
        return 0;
    }

    if (!LoadCameras(fp)) {
        return 0;
    }

    if (!LoadSoundEffects(fp)) {
        return 0;
    }

    if (!LoadBoxes(fp)) {
        return 0;
    }

    if (!LoadAnimatedTextures(fp)) {
        return 0;
    }

    if (!LoadItems(fp)) {
        return 0;
    }

    if (!LoadDepthQ(fp)) {
        return 0;
    }

    if (!LoadPalette(fp)) {
        return 0;
    }

    if (!LoadCinematic(fp)) {
        return 0;
    }

    if (!LoadDemo(fp)) {
        return 0;
    }

    if (!LoadSamples(fp)) {
        return 0;
    }

    FileClose(fp);

    HWR_DownloadTextures(TexturePageCount);

    return 1;
}

static int32_t LoadRooms(MYFILE *fp)
{
    uint16_t count2;
    uint32_t count4;

    FileRead(&RoomCount, sizeof(uint16_t), 1, fp);
    LOG_INFO("%d rooms", RoomCount);
    if (RoomCount > MAX_ROOMS) {
        strcpy(StringToShow, "LoadRoom(): Too many rooms");
        return 0;
    }

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

    return 1;
}

static int32_t LoadObjects(MYFILE *fp)
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
    for (int32_t i = 0; i < AnimCount; ++i)  //DAN
    {
        if (Anims[i].interpolation == 0)
	  {
		Anims[i].interpolation = 1;
	  }
	  else
	  {
		Anims[i].interpolation *= ANIM_SCALE;
	  }
	  Anims[i].frame_base *= ANIM_SCALE;
	  Anims[i].frame_end *= ANIM_SCALE;
	  Anims[i].jump_frame_num *= ANIM_SCALE;
	  //Anims[i].acceleration /= ANIM_SCALE;
	  //Anims[i].velocity /= ANIM_SCALE;
	}


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
    for (int32_t i = 0; i < AnimRangeCount; ++i) //DAN
    {
        AnimRanges[i].end_frame *= ANIM_SCALE;
	  AnimRanges[i].start_frame *= ANIM_SCALE;
	  AnimRanges[i].link_frame_num *= ANIM_SCALE;
    }


    FileRead(&AnimCommandCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d anim commands", AnimCommandCount);
    AnimCommands =
        game_malloc(sizeof(int16_t) * AnimCommandCount, GBUF_ANIM_COMMANDS);
    FileRead(AnimCommands, sizeof(int16_t), AnimCommandCount, fp);
       for (uint32_t j = 0; j < AnimCount; ++j)  //DAN
    {
        if (Anims[j].number_commands > 0)
        {
            int16_t* command = &AnimCommands[Anims[j].command_index];
            for (uint32_t i = Anims[j].number_commands; i > 0; i--)
            {
                switch (*(command++))
                {
                case AC_MOVE_ORIGIN:
                    /* Translate (allowing for y-axis rotation) */
                    command += 3;
                    break;

                case AC_JUMP_VELOCITY:
                    //item->fall_speed = *(command++);        // Get Upward Velocity
                    //item->speed = *(command++);         // Get Forward Velocity
                    command += 2;
                    break;

                case AC_ATTACK_READY:
                case AC_DEACTIVATE:
                    // no params
                    break;

                case AC_SOUND_FX:
                case AC_EFFECT:
                    (*command) *= ANIM_SCALE;
                    command += 2;
                    break;
                }
            }
        }
    }

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
        sprintf(StringToShow, "Too many Textures in level");
        return 0;
    }
    FileRead(PhdTextureInfo, sizeof(PHD_TEXTURE), TextureCount, fp);

    return 1;
}

static int32_t LoadSprites(MYFILE *fp)
{
    FileRead(&SpriteInfoCount, sizeof(int32_t), 1, fp);
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
    return 1;
}

static int32_t LoadItems(MYFILE *fp)
{
    int32_t item_count = 0;
    FileRead(&item_count, sizeof(int32_t), 1, fp);

    LOG_INFO("%d items", item_count);

    if (item_count) {
        if (item_count > MAX_ITEMS) {
            strcpy(StringToShow, "LoadItems(): Too Many Items being Loaded!!");
            return 0;
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
                sprintf(
                    StringToShow,
                    "LoadItems(): Bad Object number (%d) on Item %d",
                    item->object_number, i);
                S_ExitSystem(StringToShow);
            }

            InitialiseItem(i);
        }
    }

    return 1;
}

static int32_t LoadDepthQ(MYFILE *fp)
{
    LOG_INFO("");
    FileRead(DepthQTable, sizeof(uint8_t), 32 * 256, fp);
    for (int i = 0; i < 32; i++) {
        // force colour 0 to black
        DepthQTable[i][0] = 0;

        for (int j = 0; j < 256; j++) {
            GouraudTable[j][i] = DepthQTable[i][j];
        }
    }
    return 1;
}

static int32_t LoadPalette(MYFILE *fp)
{
    LOG_INFO("");
    FileRead(GamePalette, sizeof(uint8_t), 256 * 3, fp);
    GamePalette[0].r = 0;
    GamePalette[0].g = 0;
    GamePalette[0].b = 0;
    HWR_SetPalette();
    PhdWet = 0;
    for (int i = 0; i < 256; i++) {
        WaterPalette[i].r = GamePalette[i].r * 2 / 3;
        WaterPalette[i].g = GamePalette[i].g * 2 / 3;
        WaterPalette[i].b = GamePalette[i].b;
    }
    return 1;
}

static int32_t LoadCameras(MYFILE *fp)
{
    FileRead(&NumberCameras, sizeof(int32_t), 1, fp);
    LOG_INFO("%d cameras", NumberCameras);
    if (!NumberCameras) {
        return 1;
    }
    Camera.fixed =
        game_malloc(sizeof(OBJECT_VECTOR) * NumberCameras, GBUF_CAMERAS);
    if (!Camera.fixed) {
        return 0;
    }
    FileRead(Camera.fixed, sizeof(OBJECT_VECTOR), NumberCameras, fp);
    return 1;
}

static int32_t LoadSoundEffects(MYFILE *fp)
{
    FileRead(&NumberSoundEffects, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sound effects", NumberSoundEffects);
    if (!NumberSoundEffects) {
        return 1;
    }
    SoundEffectsTable =
        game_malloc(sizeof(OBJECT_VECTOR) * NumberSoundEffects, GBUF_SOUND_FX);
    if (!SoundEffectsTable) {
        return 0;
    }
    FileRead(SoundEffectsTable, sizeof(OBJECT_VECTOR), NumberSoundEffects, fp);
    return 1;
}

static int32_t LoadBoxes(MYFILE *fp)
{
    FileRead(&NumberBoxes, sizeof(int32_t), 1, fp);
    Boxes = game_malloc(sizeof(BOX_INFO) * NumberBoxes, GBUF_BOXES);
    if (!FileRead(Boxes, sizeof(BOX_INFO), NumberBoxes, fp)) {
        sprintf(StringToShow, "LoadBoxes(): Unable to load boxes");
        return 0;
    }

    FileRead(&OverlapCount, sizeof(int32_t), 1, fp);
    Overlap = (uint16_t *)game_malloc(2 * OverlapCount, 22);
    if (!FileRead(Overlap, sizeof(int16_t), OverlapCount, fp)) {
        sprintf(StringToShow, "LoadBoxes(): Unable to load box overlaps");
        return 0;
    }

    for (int i = 0; i < 2; i++) {
        GroundZone[i] =
            game_malloc(sizeof(int16_t) * NumberBoxes, GBUF_GROUNDZONE);
        if (!GroundZone[i]
            || !FileRead(GroundZone[i], sizeof(int16_t), NumberBoxes, fp)) {
            sprintf(StringToShow, "LoadBoxes(): Unable to load 'ground_zone'");
            return 0;
        }

        GroundZone2[i] = game_malloc(2 * NumberBoxes, GBUF_GROUNDZONE);
        if (!GroundZone2[i]
            || !FileRead(GroundZone2[i], sizeof(int16_t), NumberBoxes, fp)) {
            sprintf(StringToShow, "LoadBoxes(): Unable to load 'ground2_zone'");
            return 0;
        }

        FlyZone[i] = game_malloc(sizeof(int16_t) * NumberBoxes, GBUF_FLYZONE);
        if (!FlyZone[i]
            || !FileRead(FlyZone[i], sizeof(int16_t), NumberBoxes, fp)) {
            sprintf(StringToShow, "LoadBoxes(): Unable to load 'fly_zone'");
            return 0;
        }
    }

    return 1;
}

static int32_t LoadAnimatedTextures(MYFILE *fp)
{
    FileRead(&AnimTextureRangeCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d animated textures", AnimTextureRangeCount);
    AnimTextureRanges = game_malloc(
        sizeof(int16_t) * AnimTextureRangeCount, GBUF_ANIMATING_TEXTURE_RANGES);
    FileRead(AnimTextureRanges, sizeof(int16_t), AnimTextureRangeCount, fp);
    return 1;
}

static int32_t LoadCinematic(MYFILE *fp)
{
    FileRead(&NumCineFrames, sizeof(int16_t), 1, fp);
    LOG_INFO("%d cinematic frames", NumCineFrames);
    if (!NumCineFrames) {
        return 1;
    }
    Cine =
        game_malloc(sizeof(int16_t) * 8 * NumCineFrames, GBUF_CINEMATIC_FRAMES);
    FileRead(Cine, sizeof(int16_t) * 8, NumCineFrames, fp);
    return 1;
}

static int32_t LoadDemo(MYFILE *fp)
{
    DemoCount = 0;
    DemoPtr =
        game_malloc(DEMO_COUNT_MAX * sizeof(uint32_t), GBUF_LOADDEMO_BUFFER);
    uint16_t size = 0;
    FileRead(&size, sizeof(int16_t), 1, fp);
    LOG_INFO("%d demo buffer size", size);
    if (!size) {
        return 1;
    }
    FileRead(DemoPtr, 1, size, fp);
    return 1;
}

static int32_t LoadSamples(MYFILE *fp)
{
    if (!SoundIsActive) {
        return 1;
    }

    FileRead(SampleLUT, sizeof(int16_t), MAX_SAMPLES, fp);
    FileRead(&NumSampleInfos, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sample infos", NumSampleInfos);
    if (!NumSampleInfos) {
        S_ExitSystem("No Sample Infos");
        return 0;
    }

    SampleInfos =
        game_malloc(sizeof(SAMPLE_INFO) * NumSampleInfos, GBUF_SAMPLE_INFOS);
    FileRead(SampleInfos, sizeof(SAMPLE_INFO), NumSampleInfos, fp);

    int32_t sample_data_size;
    FileRead(&sample_data_size, sizeof(int32_t), 1, fp);
    LOG_INFO("%d sample data size", sample_data_size);
    if (!sample_data_size) {
        S_ExitSystem("No Sample Data");
        return 0;
    }

    char *sample_data = game_malloc(sample_data_size, GBUF_SAMPLES);
    FileRead(sample_data, sizeof(char), sample_data_size, fp);

    FileRead(&NumSamples, sizeof(int32_t), 1, fp);
    LOG_INFO("%d samples", NumSamples);
    if (!NumSamples) {
        S_ExitSystem("No Samples");
        return 0;
    }

    int32_t *sample_offsets =
        game_malloc(sizeof(int32_t) * NumSamples, GBUF_SAMPLE_OFFSETS);
    FileRead(sample_offsets, sizeof(int32_t), NumSamples, fp);

    char **sample_pointers =
        game_malloc(sizeof(char *) * NumSamples, GBUF_SAMPLE_OFFSETS);
    for (int i = 0; i < NumSamples; i++) {
        sample_pointers[i] = sample_data + sample_offsets[i];
    }

    SoundLoadSamples(sample_pointers, NumSamples);
    SoundsLoaded = 1;

    game_free(sizeof(char *) * NumSamples, GBUF_SAMPLE_OFFSETS);

    return 1;
}

static int32_t LoadTexturePages(MYFILE *fp)
{
    FileRead(&TexturePageCount, sizeof(int32_t), 1, fp);
    LOG_INFO("%d texture pages", TexturePageCount);
    int8_t *base = game_malloc(TexturePageCount * 65536, GBUF_TEXTURE_PAGES);
    FileRead(base, 65536, TexturePageCount, fp);
    for (int i = 0; i < TexturePageCount; i++) {
        TexturePagePtrs[i] = base;
        base += 65536;
    }
    return 1;
}

int32_t S_LoadLevel(int level_num)
{
    LOG_INFO("%d (%s)", level_num, GF.levels[level_num].level_file);
    int32_t ret = LoadLevel(GF.levels[level_num].level_file, level_num);

    if (T1MConfig.disable_healing_between_levels) {
        // check if we're in main menu by seeing if there is Lara item in the
        // currently loaded level.
        int8_t lara_found = 0;
        int8_t in_cutscene = 0;
        for (int i = 0; i < LevelItemCount; i++) {
            if (Items[i].object_number == O_LARA) {
                lara_found = 1;
            }
            if (Items[i].object_number == O_PLAYER_1
                || Items[i].object_number == O_PLAYER_2
                || Items[i].object_number == O_PLAYER_3
                || Items[i].object_number == O_PLAYER_4) {
                in_cutscene = 1;
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

void T1MInjectSpecificFile()
{
    INJECT(0x0041AF90, S_LoadLevel);
    INJECT(0x0041AFB0, LoadLevel);
    INJECT(0x0041B3F0, LoadRooms);
    INJECT(0x0041B710, LoadObjects);
    INJECT(0x0041BB50, LoadSprites);
    INJECT(0x0041BC60, LoadItems);
    INJECT(0x0041BE00, LoadBoxes);
    INJECT(0x0041BFC0, GetFullPath);
    INJECT(0x00438390, FileLoad);
}
