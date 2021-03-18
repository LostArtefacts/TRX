#include "specific/file.h"

#include "game/control.h"
#include "game/items.h"
#include "game/setup.h"
#include "game/vars.h"
#include "specific/init.h"
#include "specific/shed.h"

#include "config.h"
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

int32_t LoadLevel(const char *filename, int32_t level_num)
{
    int32_t version;
    int32_t file_level_num;

    const char *full_path = GetFullPath(filename);
    TRACE("%s", full_path);

    init_game_malloc();
    FILE *fp = fopen(full_path, "rb");
    if (!fp) {
        sprintf(StringToShow, "S_LoadLevel(): Could not open %s", full_path);
        S_ExitSystem(StringToShow);
        return 0;
    }

    fread(&version, sizeof(int32_t), 1, fp);
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

    fread(&file_level_num, sizeof(int32_t), 1, fp);
    TRACE("file level num: %d", file_level_num);

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

    fclose(fp);

    if (IsHardwareRenderer) {
        DownloadTexturesToHardware(TexturePageCount);
    }

    return 1;
}

int32_t LoadRooms(FILE *fp)
{
    uint16_t count2;
    uint32_t count4;

    fread(&RoomCount, sizeof(uint16_t), 1, fp);
    TRACE("%d rooms", RoomCount);
    if (RoomCount > MAX_ROOMS) {
        strcpy(StringToShow, "LoadRoom(): Too many rooms");
        return 0;
    }

    RoomInfo = game_malloc(sizeof(ROOM_INFO) * RoomCount, GBUF_ROOM_INFOS);
    int i = 0;
    for (ROOM_INFO *current_room_info = RoomInfo; i < RoomCount;
         i++, current_room_info++) {
        // Room position
        fread(&current_room_info->x, sizeof(uint32_t), 1, fp);
        current_room_info->y = 0;
        fread(&current_room_info->z, sizeof(uint32_t), 1, fp);

        // Room floor/ceiling
        fread(&current_room_info->min_floor, sizeof(uint32_t), 1, fp);
        fread(&current_room_info->max_ceiling, sizeof(uint32_t), 1, fp);

        // Room mesh
        fread(&count4, sizeof(uint32_t), 1, fp);
        current_room_info->data =
            game_malloc(sizeof(uint16_t) * count4, GBUF_ROOM_MESH);
        fread(current_room_info->data, sizeof(uint16_t), count4, fp);

        // Doors
        fread(&count2, sizeof(uint16_t), 1, fp);
        if (!count2) {
            current_room_info->doors = NULL;
        } else {
            current_room_info->doors = game_malloc(
                sizeof(uint16_t) + sizeof(DOOR_INFO) * count2, GBUF_ROOM_DOOR);
            current_room_info->doors->count = count2;
            fread(
                &current_room_info->doors->door, sizeof(DOOR_INFO), count2, fp);
        }

        // Room floor
        fread(&current_room_info->x_size, sizeof(uint16_t), 1, fp);
        fread(&current_room_info->y_size, sizeof(uint16_t), 1, fp);
        count4 = current_room_info->y_size * current_room_info->x_size;
        current_room_info->floor =
            game_malloc(sizeof(FLOOR_INFO) * count4, GBUF_ROOM_FLOOR);
        fread(current_room_info->floor, sizeof(FLOOR_INFO), count4, fp);

        // Room lights
        fread(&current_room_info->ambient, sizeof(uint16_t), 1, fp);
        fread(&current_room_info->num_lights, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_lights) {
            current_room_info->light = NULL;
        } else {
            current_room_info->light = game_malloc(
                sizeof(LIGHT_INFO) * current_room_info->num_lights,
                GBUF_ROOM_LIGHTS);
            fread(
                current_room_info->light, sizeof(LIGHT_INFO),
                current_room_info->num_lights, fp);
        }

        // Static mesh infos
        fread(&current_room_info->num_meshes, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_meshes) {
            current_room_info->mesh = NULL;
        } else {
            current_room_info->mesh = game_malloc(
                sizeof(MESH_INFO) * current_room_info->num_meshes,
                GBUF_ROOM_STATIC_MESH_INFOS);
            fread(
                current_room_info->mesh, sizeof(MESH_INFO),
                current_room_info->num_meshes, fp);
        }

        // Flipped (alternative) room
        fread(&current_room_info->flipped_room, sizeof(uint16_t), 1, fp);

        // Room flags
        fread(&current_room_info->flags, sizeof(uint16_t), 1, fp);

        // Initialise some variables
        current_room_info->bound_active = 0;
        current_room_info->left = PhdWinMaxX;
        current_room_info->top = PhdWinMaxY;
        current_room_info->bottom = 0;
        current_room_info->right = 0;
        current_room_info->item_number = -1;
        current_room_info->fx_number = -1;
    }

    fread(&FloorDataSize, sizeof(uint32_t), 1, fp);
    FloorData = game_malloc(sizeof(uint16_t) * FloorDataSize, GBUF_FLOOR_DATA);
    fread(FloorData, sizeof(uint16_t), FloorDataSize, fp);

    return 1;
}

int32_t LoadObjects(FILE *fp)
{
    fread(&MeshCount, sizeof(int32_t), 1, fp);
    TRACE("%d meshes", MeshCount);
    MeshBase = game_malloc(sizeof(int16_t) * MeshCount, GBUF_MESHES);
    fread(MeshBase, sizeof(int16_t), MeshCount, fp);

    fread(&MeshPtrCount, sizeof(int32_t), 1, fp);
    uint32_t *mesh_indices =
        game_malloc(sizeof(uint32_t) * MeshPtrCount, GBUF_MESH_POINTERS);
    fread(mesh_indices, sizeof(uint32_t), MeshPtrCount, fp);

    Meshes = game_malloc(sizeof(int16_t *) * MeshPtrCount, GBUF_MESH_POINTERS);
    for (int i = 0; i < MeshPtrCount; i++) {
        Meshes[i] = &MeshBase[mesh_indices[i] / 2];
    }

    fread(&AnimCount, sizeof(int32_t), 1, fp);
    TRACE("%d anims", AnimCount);
    Anims = game_malloc(sizeof(ANIM_STRUCT) * AnimCount, GBUF_ANIMS);
    fread(Anims, sizeof(ANIM_STRUCT), AnimCount, fp);

    fread(&AnimChangeCount, sizeof(int32_t), 1, fp);
    TRACE("%d anim changes", AnimChangeCount);
    AnimChanges = game_malloc(
        sizeof(ANIM_CHANGE_STRUCT) * AnimChangeCount, GBUF_ANIM_CHANGES);
    fread(AnimChanges, sizeof(ANIM_CHANGE_STRUCT), AnimChangeCount, fp);

    fread(&AnimRangeCount, sizeof(int32_t), 1, fp);
    TRACE("%d anim ranges", AnimRangeCount);
    AnimRanges = game_malloc(
        sizeof(ANIM_RANGE_STRUCT) * AnimRangeCount, GBUF_ANIM_RANGES);
    fread(AnimRanges, sizeof(ANIM_RANGE_STRUCT), AnimRangeCount, fp);

    fread(&AnimCommandCount, sizeof(int32_t), 1, fp);
    TRACE("%d anim commands", AnimCommandCount);
    AnimCommands =
        game_malloc(sizeof(int16_t) * AnimCommandCount, GBUF_ANIM_COMMANDS);
    fread(AnimCommands, sizeof(int16_t), AnimCommandCount, fp);

    fread(&AnimBoneCount, sizeof(int32_t), 1, fp);
    TRACE("%d anim bones", AnimBoneCount);
    AnimBones = game_malloc(sizeof(int32_t) * AnimBoneCount, GBUF_ANIM_BONES);
    fread(AnimBones, sizeof(int32_t), AnimBoneCount, fp);

    fread(&AnimFrameCount, sizeof(int32_t), 1, fp);
    TRACE("%d anim frames", AnimFrameCount);
    AnimFrames =
        game_malloc(sizeof(int16_t) * AnimFrameCount, GBUF_ANIM_FRAMES);
    fread(AnimFrames, sizeof(int16_t), AnimFrameCount, fp);
    for (int i = 0; i < AnimCount; i++) {
        Anims[i].frame_ptr = &AnimFrames[(size_t)Anims[i].frame_ptr / 2];
    }

    fread(&ObjectCount, sizeof(int32_t), 1, fp);
    TRACE("%d objects", ObjectCount);
    for (int i = 0; i < ObjectCount; i++) {
        int32_t tmp;
        fread(&tmp, sizeof(int32_t), 1, fp);
        OBJECT_INFO *object = &Objects[tmp];

        fread(&object->nmeshes, sizeof(int16_t), 1, fp);
        fread(&object->mesh_index, sizeof(int16_t), 1, fp);
        fread(&object->bone_index, sizeof(int32_t), 1, fp);

        fread(&tmp, sizeof(int32_t), 1, fp);
        object->frame_base = &AnimFrames[tmp / 2];
        fread(&object->anim_index, sizeof(int16_t), 1, fp);
        object->loaded = 1;
    }

    InitialiseObjects();

    fread(&StaticCount, sizeof(int32_t), 1, fp);
    TRACE("%d statics", StaticCount);
    for (int i = 0; i < StaticCount; i++) {
        int32_t tmp;
        fread(&tmp, sizeof(int32_t), 1, fp);
        STATIC_INFO *object = &StaticObjects[tmp];

        fread(&object->mesh_number, sizeof(int16_t), 1, fp);
        fread(&object->x_minp, sizeof(int16_t), 6, fp);
        fread(&object->x_minc, sizeof(int16_t), 6, fp);
        fread(&object->flags, sizeof(int16_t), 1, fp);
    }

    fread(&TextureCount, sizeof(int32_t), 1, fp);
    TRACE("%d textures", TextureCount);
    if (TextureCount > MAX_TEXTURES) {
        sprintf(StringToShow, "Too many Textures in level");
        return 0;
    }
    fread(PhdTextInfo, sizeof(PHDTEXTURESTRUCT), TextureCount, fp);

    return 1;
}

int32_t LoadSprites(FILE *fp)
{
    fread(&SpriteInfoCount, sizeof(int32_t), 1, fp);
    fread(&PhdSpriteInfo, sizeof(PHDSPRITESTRUCT), SpriteInfoCount, fp);

    fread(&SpriteCount, sizeof(int32_t), 1, fp);
    for (int i = 0; i < SpriteCount; i++) {
        int32_t object_num;
        fread(&object_num, sizeof(int32_t), 1, fp);
        if (object_num < NUMBER_OBJECTS) {
            fread(&Objects[object_num], sizeof(int16_t), 1, fp);
            fread(&Objects[object_num].mesh_index, sizeof(int16_t), 1, fp);
            Objects[object_num].loaded = 1;
        } else {
            int32_t static_num = object_num - NUMBER_OBJECTS;
            fseek(fp, 2, 1);
            fread(
                &StaticObjects[static_num].mesh_number, sizeof(int16_t), 1, fp);
        }
    }
    return 1;
}

int32_t LoadItems(FILE *fp)
{
    int32_t item_count = 0;
    fread(&item_count, sizeof(int32_t), 1, fp);

    TRACE("%d items", item_count);

    if (item_count) {
        if (item_count > NUMBER_ITEMS) {
            strcpy(StringToShow, "LoadItems(): Too Many Items being Loaded!!");
            return 0;
        }

        Items = game_malloc(sizeof(ITEM_INFO) * NUMBER_ITEMS, GBUF_ITEMS);
        LevelItemCount = item_count;
        InitialiseItemArray(NUMBER_ITEMS);

        for (int i = 0; i < item_count; i++) {
            ITEM_INFO *item = &Items[i];
            fread(&item->object_number, sizeof(int16_t), 1, fp);
            fread(&item->room_number, sizeof(int16_t), 1, fp);
            fread(&item->pos.x, sizeof(int32_t), 1, fp);
            fread(&item->pos.y, sizeof(int32_t), 1, fp);
            fread(&item->pos.z, sizeof(int32_t), 1, fp);
            fread(&item->pos.y_rot, sizeof(int16_t), 1, fp);
            fread(&item->shade, sizeof(int16_t), 1, fp);
            fread(&item->flags, sizeof(uint16_t), 1, fp);

            if (item->object_number < 0
                || item->object_number >= NUMBER_OBJECTS) {
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

int32_t LoadDepthQ(FILE *fp)
{
    TRACE("");
    fread(DepthQTable, sizeof(uint8_t), 32 * 256, fp);
    for (int i = 0; i < 32; i++) {
        // force colour 0 to black
        DepthQTable[i][0] = 0;

        for (int j = 0; j < 256; j++) {
            GouraudTable[j][i] = DepthQTable[i][j];
        }
    }
    return 1;
}

int32_t LoadPalette(FILE *fp)
{
    TRACE("");
    fread(GamePalette, sizeof(uint8_t), 256 * 3, fp);
    GamePalette[0] = 0;
    GamePalette[1] = 0;
    GamePalette[2] = 0;
    if (IsHardwareRenderer) {
        PaletteSetHardware();
    }
    PhdWet = 0;
    for (int i = 0; i < 256 * 3; i += 3) {
        WaterPalette[i + 0] = GamePalette[i + 0] * 2 / 3;
        WaterPalette[i + 1] = GamePalette[i + 1] * 2 / 3;
        WaterPalette[i + 2] = GamePalette[i + 2];
    }
    return 1;
}

int32_t LoadCameras(FILE *fp)
{
    fread(&NumberCameras, sizeof(int32_t), 1, fp);
    TRACE("%d cameras", NumberCameras);
    if (!NumberCameras) {
        return 1;
    }
    Camera.fixed =
        game_malloc(sizeof(OBJECT_VECTOR) * NumberCameras, GBUF_CAMERAS);
    if (!Camera.fixed) {
        return 0;
    }
    fread(Camera.fixed, sizeof(OBJECT_VECTOR), NumberCameras, fp);
    return 1;
}

int32_t LoadSoundEffects(FILE *fp)
{
    fread(&NumberSoundEffects, sizeof(int32_t), 1, fp);
    TRACE("%d sound effects", NumberSoundEffects);
    if (!NumberSoundEffects) {
        return 1;
    }
    SoundEffectsTable =
        game_malloc(sizeof(OBJECT_VECTOR) * NumberSoundEffects, GBUF_SOUND_FX);
    if (!SoundEffectsTable) {
        return 0;
    }
    fread(SoundEffectsTable, sizeof(OBJECT_VECTOR), NumberSoundEffects, fp);
    return 1;
}

int32_t LoadBoxes(FILE *fp)
{
    fread(&NumberBoxes, sizeof(int32_t), 1, fp);
    Boxes = game_malloc(sizeof(BOX_INFO) * NumberBoxes, GBUF_BOXES);
    if (!fread(Boxes, sizeof(BOX_INFO), NumberBoxes, fp)) {
        sprintf(StringToShow, "LoadBoxes(): Unable to load boxes");
        return 0;
    }

    fread(&OverlapCount, sizeof(int32_t), 1, fp);
    Overlap = (uint16_t *)game_malloc(2 * OverlapCount, 22);
    if (!fread(Overlap, sizeof(int16_t), OverlapCount, fp)) {
        sprintf(StringToShow, "LoadBoxes(): Unable to load box overlaps");
        return 0;
    }

    for (int i = 0; i < 2; i++) {
        GroundZone[i] =
            game_malloc(sizeof(int16_t) * NumberBoxes, GBUF_GROUNDZONE);
        if (!GroundZone[i]
            || !fread(GroundZone[i], sizeof(int16_t), NumberBoxes, fp)) {
            sprintf(StringToShow, "LoadBoxes(): Unable to load 'ground_zone'");
            return 0;
        }

        GroundZone2[i] = game_malloc(2 * NumberBoxes, GBUF_GROUNDZONE);
        if (!GroundZone2[i]
            || !fread(GroundZone2[i], sizeof(int16_t), NumberBoxes, fp)) {
            sprintf(StringToShow, "LoadBoxes(): Unable to load 'ground2_zone'");
            return 0;
        }

        FlyZone[i] = game_malloc(sizeof(int16_t) * NumberBoxes, GBUF_FLYZONE);
        if (!FlyZone[i]
            || !fread(FlyZone[i], sizeof(int16_t), NumberBoxes, fp)) {
            sprintf(StringToShow, "LoadBoxes(): Unable to load 'fly_zone'");
            return 0;
        }
    }

    return 1;
}

int32_t LoadAnimatedTextures(FILE *fp)
{
    fread(&AnimTextureRangeCount, sizeof(int32_t), 1, fp);
    TRACE("%d animated textures", AnimTextureRangeCount);
    AnimTextureRanges = game_malloc(
        sizeof(int16_t) * AnimTextureRangeCount, GBUF_ANIMATING_TEXTURE_RANGES);
    fread(AnimTextureRanges, sizeof(int16_t), AnimTextureRangeCount, fp);
    return 1;
}

int32_t LoadCinematic(FILE *fp)
{
    fread(&NumCineFrames, sizeof(int16_t), 1, fp);
    TRACE("%d cinematic frames", NumCineFrames);
    if (!NumCineFrames) {
        return 1;
    }
    Cine =
        game_malloc(sizeof(int16_t) * 8 * NumCineFrames, GBUF_CINEMATIC_FRAMES);
    fread(Cine, sizeof(int16_t) * 8, NumCineFrames, fp);
    return 1;
}

int32_t LoadDemo(FILE *fp)
{
    DemoCount = 0;
    DemoPtr =
        game_malloc(DEMO_COUNT_MAX * sizeof(uint32_t), GBUF_LOADDEMO_BUFFER);
    uint16_t size = 0;
    fread(&size, sizeof(int16_t), 1, fp);
    TRACE("%d demo buffer size", size);
    if (!size) {
        return 1;
    }
    fread(DemoPtr, 1, size, fp);
    return 1;
}

int32_t LoadSamples(FILE *fp)
{
    if (!SoundIsActive) {
        return 1;
    }

    fread(SampleLUT, sizeof(int16_t), MAX_SAMPLES, fp);
    fread(&NumSampleInfos, sizeof(int32_t), 1, fp);
    TRACE("%d sample infos", NumSampleInfos);
    if (!NumSampleInfos) {
        S_ExitSystem("No Sample Infos");
        return 0;
    }

    SampleInfos = game_malloc(8 * NumSampleInfos, GBUF_SAMPLE_INFOS);
    fread(SampleInfos, sizeof(SAMPLE_INFO), NumSampleInfos, fp);

    int32_t sample_data_size;
    TRACE("%d sample data size", sample_data_size);
    fread(&sample_data_size, sizeof(int32_t), 1, fp);
    if (!sample_data_size) {
        S_ExitSystem("No Sample Data");
    }

    char *sample_data = game_malloc(sample_data_size, GBUF_SAMPLES);
    fread(sample_data, sizeof(int16_t), sample_data_size / sizeof(int16_t), fp);

    fread(&NumSamples, sizeof(int32_t), 1u, fp);
    TRACE("%d samples", NumSamples);
    if (!NumSamples) {
        S_ExitSystem("No Samples");
    }

    int32_t *sample_offsets =
        game_malloc(sizeof(int32_t) * NumSamples, GBUF_SAMPLE_OFFSETS);
    fread(sample_offsets, sizeof(int32_t), NumSamples, fp);

    char **sample_pointers =
        game_malloc(sizeof(char *) * (NumSamples + 1), GBUF_SAMPLE_OFFSETS);
    sample_pointers[0] = (char *)&SampleLoader;
    for (int i = 0; i < NumSamples; i++) {
        sample_pointers[i + 1] = sample_data + sample_offsets[i];
    }

    WinSndLoadSamples(sample_pointers, -NumSamples);
    SoundsLoaded = 1;

    game_free(sizeof(char *) * (NumSamples + 1), GBUF_SAMPLE_OFFSETS);

    return 1;
}

int32_t LoadTexturePages(FILE *fp)
{
    fread(&TexturePageCount, sizeof(int32_t), 1, fp);
    TRACE("%d texture pages", TexturePageCount);
    int8_t *base = game_malloc(TexturePageCount * 65536, GBUF_TEXTURE_PAGES);
    fread(base, 65536, TexturePageCount, fp);
    for (int i = 0; i < TexturePageCount; i++) {
        TexturePagePtrs[i] = base;
        base += 65536;
    }
    return 1;
}

int32_t S_LoadLevel(int level_num)
{
    TRACE("%d (%s)", level_num, GF.levels[level_num].level_file);
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
    TRACE("%s", filename);
    sprintf(newpath, ".\\%s", filename);
    return newpath;
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
}
