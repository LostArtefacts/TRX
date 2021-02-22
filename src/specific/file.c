#include "game/vars.h"
#include "game/items.h"
#include "game/setup.h"
#include "specific/file.h"
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

int32_t __cdecl LoadRooms(FILE* fp)
{
    TRACE("");
    uint16_t count2;
    uint32_t count4;

    _fread(&RoomCount, sizeof(uint16_t), 1, fp);
    if (RoomCount > MAX_ROOMS) {
        strcpy(StringToShow, "LoadRoom(): Too many rooms");
        return 0;
    }

    RoomInfo = game_malloc(sizeof(ROOM_INFO) * RoomCount, GBUF_ROOM_INFOS);
    if (!RoomInfo) {
        strcpy(StringToShow, "LoadRoom(): Could not allocate memory for rooms");
        return 0;
    }

    int i = 0;
    for (ROOM_INFO* current_room_info = RoomInfo; i < RoomCount;
         ++i, ++current_room_info) {
        // Room position
        _fread(&current_room_info->x, sizeof(uint32_t), 1, fp);
        current_room_info->y = 0;
        _fread(&current_room_info->z, sizeof(uint32_t), 1, fp);

        // Room floor/ceiling
        _fread(&current_room_info->min_floor, sizeof(uint32_t), 1, fp);
        _fread(&current_room_info->max_ceiling, sizeof(uint32_t), 1, fp);

        // Room mesh
        _fread(&count4, sizeof(uint32_t), 1, fp);
        current_room_info->data =
            game_malloc(sizeof(uint16_t) * count4, GBUF_ROOM_MESH);
        _fread(current_room_info->data, sizeof(uint16_t), count4, fp);

        // Doors
        _fread(&count2, sizeof(uint16_t), 1, fp);
        if (!count2) {
            current_room_info->doors = NULL;
        } else {
            current_room_info->doors = game_malloc(
                sizeof(uint16_t) + sizeof(DOOR_INFO) * count2, GBUF_ROOM_DOOR);
            current_room_info->doors->count = count2;
            _fread(
                &current_room_info->doors->door, sizeof(DOOR_INFO), count2, fp);
        }

        // Room floor
        _fread(&current_room_info->x_size, sizeof(uint16_t), 1, fp);
        _fread(&current_room_info->y_size, sizeof(uint16_t), 1, fp);
        count4 = current_room_info->y_size * current_room_info->x_size;
        current_room_info->floor =
            game_malloc(sizeof(FLOOR_INFO) * count4, GBUF_ROOM_FLOOR);
        _fread(current_room_info->floor, sizeof(FLOOR_INFO), count4, fp);

        // Room lights
        _fread(&current_room_info->ambient, sizeof(uint16_t), 1, fp);
        _fread(&current_room_info->num_lights, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_lights) {
            current_room_info->light = NULL;
        } else {
            current_room_info->light = game_malloc(
                sizeof(LIGHT_INFO) * current_room_info->num_lights,
                GBUF_ROOM_LIGHTS);
            _fread(
                current_room_info->light, sizeof(LIGHT_INFO),
                current_room_info->num_lights, fp);
        }

        // Static mesh infos
        _fread(&current_room_info->num_meshes, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_meshes) {
            current_room_info->mesh = NULL;
        } else {
            current_room_info->mesh = game_malloc(
                sizeof(MESH_INFO) * current_room_info->num_meshes,
                GBUF_ROOM_STATIC_MESH_INFOS);
            _fread(
                current_room_info->mesh, sizeof(MESH_INFO),
                current_room_info->num_meshes, fp);
        }

        // Flipped (alternative) room
        _fread(&current_room_info->flipped_room, sizeof(uint16_t), 1, fp);

        // Room flags
        _fread(&current_room_info->flags, sizeof(uint16_t), 1, fp);

        // Initialise some variables
        current_room_info->bound_active = 0;
        current_room_info->left = PhdWinMaxX;
        current_room_info->top = PhdWinMaxY;
        current_room_info->bottom = 0;
        current_room_info->right = 0;
        current_room_info->item_number = -1;
        current_room_info->fx_number = -1;
    }

    _fread(&FloorDataSize, sizeof(uint32_t), 1, fp);
    FloorData = game_malloc(sizeof(uint16_t) * FloorDataSize, GBUF_FLOOR_DATA);
    _fread(FloorData, sizeof(uint16_t), FloorDataSize, fp);

    return 1;
}

int32_t __cdecl LoadObjects(FILE* handle)
{
    _fread(&MeshCount, sizeof(int32_t), 1, handle);
    MeshBase = game_malloc(sizeof(int16_t) * MeshCount, GBUF_MESHES);
    _fread(MeshBase, sizeof(int16_t), MeshCount, handle);

    _fread(&MeshPtrCount, sizeof(int32_t), 1, handle);
    uint32_t* mesh_indices =
        game_malloc(sizeof(uint32_t) * MeshPtrCount, GBUF_MESH_POINTERS);
    _fread(mesh_indices, sizeof(uint32_t), MeshPtrCount, handle);

    Meshes = game_malloc(sizeof(int16_t*) * MeshPtrCount, GBUF_MESH_POINTERS);
    for (int i = 0; i < MeshPtrCount; i++) {
        Meshes[i] = &MeshBase[mesh_indices[i] / 2];
    }

    _fread(&AnimCount, sizeof(int32_t), 1, handle);
    Anims = game_malloc(sizeof(ANIM_STRUCT) * AnimCount, GBUF_ANIMS);
    _fread(Anims, sizeof(ANIM_STRUCT), AnimCount, handle);

    _fread(&AnimChangeCount, sizeof(int32_t), 1, handle);
    AnimChanges = game_malloc(
        sizeof(ANIM_CHANGE_STRUCT) * AnimChangeCount, GBUF_ANIM_CHANGES);
    _fread(AnimChanges, sizeof(ANIM_CHANGE_STRUCT), AnimChangeCount, handle);

    _fread(&AnimRangeCount, sizeof(int32_t), 1, handle);
    AnimRanges = game_malloc(
        sizeof(ANIM_RANGE_STRUCT) * AnimRangeCount, GBUF_ANIM_RANGES);
    _fread(AnimRanges, sizeof(ANIM_RANGE_STRUCT), AnimRangeCount, handle);

    _fread(&AnimCommandCount, sizeof(int32_t), 1, handle);
    AnimCommands =
        game_malloc(sizeof(int16_t) * AnimCommandCount, GBUF_ANIM_COMMANDS);
    _fread(AnimCommands, sizeof(int16_t), AnimCommandCount, handle);

    _fread(&AnimBoneCount, sizeof(int32_t), 1, handle);
    AnimBones = game_malloc(sizeof(int32_t) * AnimBoneCount, GBUF_ANIM_BONES);
    _fread(AnimBones, sizeof(int32_t), AnimBoneCount, handle);

    _fread(&AnimFrameCount, sizeof(int32_t), 1, handle);
    AnimFrames =
        game_malloc(sizeof(int16_t) * AnimFrameCount, GBUF_ANIM_FRAMES);
    _fread(AnimFrames, sizeof(int16_t), AnimFrameCount, handle);
    for (int i = 0; i < AnimCount; i++) {
        Anims[i].frame_ptr = &AnimFrames[(size_t)Anims[i].frame_ptr / 2];
    }

    _fread(&ObjectCount, sizeof(int32_t), 1, handle);
    for (int i = 0; i < ObjectCount; i++) {
        int32_t tmp;
        _fread(&tmp, sizeof(int32_t), 1, handle);
        OBJECT_INFO* object = &Objects[tmp];

        _fread(&object->nmeshes, sizeof(int16_t), 1, handle);
        _fread(&object->mesh_index, sizeof(int16_t), 1, handle);
        _fread(&object->bone_index, sizeof(int32_t), 1, handle);

        _fread(&tmp, sizeof(int32_t), 1, handle);
        object->frame_base = &AnimFrames[tmp / 2];
        _fread(&object->anim_index, sizeof(int16_t), 1, handle);
        object->loaded = 1;
    }

    InitialiseObjects();

    _fread(&StaticCount, sizeof(int32_t), 1, handle);
    for (int i = 0; i < StaticCount; i++) {
        int32_t tmp;
        _fread(&tmp, sizeof(int32_t), 1, handle);
        STATIC_INFO* object = &StaticObjects[tmp];

        _fread(&object->mesh_number, sizeof(int16_t), 1, handle);
        _fread(&object->x_minp, sizeof(int16_t), 6, handle);
        _fread(&object->x_minc, sizeof(int16_t), 6, handle);
        _fread(&object->flags, sizeof(int16_t), 1, handle);
    }

    _fread(&TextureCount, sizeof(int32_t), 1, handle);
    if (TextureCount > MAX_TEXTURES) {
        sprintf(StringToShow, "Too many Textures in level");
        return 0;
    }
    _fread(PhdTextInfo, sizeof(PHDTEXTURESTRUCT), TextureCount, handle);

    return 1;
}

int32_t __cdecl LoadItems(FILE* handle)
{
    int32_t item_count = 0;
    _fread(&item_count, sizeof(int32_t), 1, handle);

    TRACE("Item count: %d", item_count);

    if (item_count) {
        if (item_count > NUMBER_ITEMS) {
            strcpy(StringToShow, "LoadItems(): Too Many Items being Loaded!!");
            return 0;
        }

        Items = game_malloc(sizeof(ITEM_INFO) * NUMBER_ITEMS, GBUF_ITEMS);
        if (!Items) {
            strcpy(
                StringToShow,
                "LoadItems(): Unable to allocate memory for 'items'");
            return 0;
        }

        LevelItemCount = item_count;
        InitialiseItemArray(NUMBER_ITEMS);

        for (int i = 0; i < item_count; ++i) {
            ITEM_INFO* item = &Items[i];
            _fread(&item->object_number, 2, 1, handle);
            _fread(&item->room_number, 2, 1, handle);
            _fread(&item->pos.x, 4, 1, handle);
            _fread(&item->pos.y, 4, 1, handle);
            _fread(&item->pos.z, 4, 1, handle);
            _fread(&item->pos.y_rot, 2, 1, handle);
            _fread(&item->shade, 2, 1, handle);
            _fread(&item->flags, 2, 1, handle);

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

int32_t __cdecl S_LoadLevel(int level_id)
{
    TRACE("%d (%s)", level_id, LevelNames[level_id]);
    int32_t ret = LoadLevel(LevelNames[level_id], level_id);

#ifdef TOMB1M_FEAT_GAMEPLAY
    if (T1MConfig.disable_healing_between_levels) {
        // check if we're in main menu by seeing if there is Lara item in the
        // currently loaded level.
        int lara_found = 0;
        int in_cutscene = 0;
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
#endif

#ifdef TOMB1M_FEAT_LEVEL_FIXES
    if (T1MConfig.fix_pyramid_secret_trigger) {
        FixPyramidSecretTrigger();
    }
    if (T1MConfig.fix_hardcoded_secret_counts) {
        SecretTotals[level_id] = GetSecretCount();
    }
#endif

    return ret;
}

const char* __cdecl GetFullPath(const char* filename)
{
    TRACE("%s", filename);
#ifdef TOMB1M_FEAT_NOCD
    sprintf(newpath, ".\\%s", filename);
#else
    if (DEMO) {
        sprintf(newpath, "%c:\\tomb\\%s", cd_drive, filename);
    } else {
        sprintf(newpath, "%c:\\%s", cd_drive, filename);
    }
#endif
    return newpath;
}

void __cdecl FindCdDrive()
{
    TRACE("");
#ifdef TOMB1M_FEAT_NOCD
    return;
#endif
    FILE* fp;
    char root[5] = "C:\\";
    char tmp_path[MAX_PATH];

    for (cd_drive = 'C'; cd_drive <= 'Z'; cd_drive++) {
        root[0] = cd_drive;
        if (GetDriveType(root) == DRIVE_CDROM) {
            sprintf(tmp_path, "%c:\\tomb\\data\\title.phd", cd_drive);
            fp = fopen(tmp_path, "rb");
            if (fp) {
                DEMO = 1;
                fclose(fp);
                return;
            }

            sprintf(tmp_path, "%c:\\data\\title.phd", cd_drive);
            fp = fopen(tmp_path, "rb");
            if (fp) {
                DEMO = 0;
                fclose(fp);
                return;
            }
        }
    }

    ShowFatalError("ERROR: Please insert TombRaider CD");
}

#ifdef TOMB1M_FEAT_LEVEL_FIXES
int GetSecretCount()
{
    int count = 0;
    uint32_t secrets = 0;

    for (int i = 0; i < RoomCount; i++) {
        ROOM_INFO* r = &RoomInfo[i];
        FLOOR_INFO* floor = &r->floor[0];
        for (int j = 0; j < r->y_size * r->x_size; j++, floor++) {
            int k = floor->index;
            if (!k) {
                continue;
            }

            while (1) {
                uint16_t floor = FloorData[k++];

                switch (floor & DATA_TYPE) {
                case FT_DOOR:
                case FT_ROOF:
                case FT_TILT:
                    k++;
                    break;

                case FT_LAVA:
                    break;

                case FT_TRIGGER: {
                    uint16_t trig_type = (floor & 0x3F00) >> 8;
                    k++; // skip basic trigger stuff

                    if (trig_type == TT_SWITCH || trig_type == TT_KEY
                        || trig_type == TT_PICKUP) {
                        k++;
                    }

                    while (1) {
                        int16_t command = FloorData[k++];
                        if (TRIG_BITS(command) == TO_CAMERA) {
                            k++;
                        } else if (TRIG_BITS(command) == TO_SECRET) {
                            int16_t number = command & VALUE_BITS;
                            if (!(secrets & (1 << number))) {
                                secrets |= (1 << number);
                                count++;
                            }
                        }

                        if (command & END_BIT) {
                            break;
                        }
                    }
                    break;
                }
                }

                if (floor & END_BIT) {
                    break;
                }
            }
        }
    }

    return count;
}

void FixPyramidSecretTrigger()
{
    if (CurrentLevel != LV_LEVEL10C) {
        return;
    }

    uint32_t global_secrets = 0;

    for (int i = 0; i < RoomCount; i++) {
        uint32_t room_secrets = 0;
        ROOM_INFO* r = &RoomInfo[i];
        FLOOR_INFO* floor = &r->floor[0];
        for (int j = 0; j < r->y_size * r->x_size; j++, floor++) {
            int k = floor->index;
            if (!k) {
                continue;
            }

            while (1) {
                uint16_t floor = FloorData[k++];

                switch (floor & DATA_TYPE) {
                case FT_DOOR:
                case FT_ROOF:
                case FT_TILT:
                    k++;
                    break;

                case FT_LAVA:
                    break;

                case FT_TRIGGER: {
                    uint16_t trig_type = (floor & 0x3F00) >> 8;
                    k++; // skip basic trigger stuff

                    if (trig_type == TT_SWITCH || trig_type == TT_KEY
                        || trig_type == TT_PICKUP) {
                        k++;
                    }

                    while (1) {
                        int16_t* command = &FloorData[k++];
                        if (TRIG_BITS(*command) == TO_CAMERA) {
                            k++;
                        } else if (TRIG_BITS(*command) == TO_SECRET) {
                            int16_t number = *command & VALUE_BITS;
                            if (global_secrets & (1 << number) && number == 0) {
                                // the secret number was already used.
                                // update the number to 2.
                                *command |= 2;
                            } else {
                                room_secrets |= (1 << number);
                            }
                        }

                        if (*command & END_BIT) {
                            break;
                        }
                    }
                    break;
                }
                }

                if (floor & END_BIT) {
                    break;
                }
            }
        }
        global_secrets |= room_secrets;
    }
}
#endif

void T1MInjectSpecificFile()
{
    INJECT(0x0041B3F0, LoadRooms);
    INJECT(0x0041B710, LoadObjects);
    INJECT(0x0041BC60, LoadItems);
    INJECT(0x0041AF90, S_LoadLevel);
    INJECT(0x0041BFC0, GetFullPath);
    INJECT(0x0041C020, FindCdDrive);
}
