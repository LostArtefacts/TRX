#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "data.h"
#include "func.h"
#include "mod.h"
#include "struct.h"

void __cdecl init_game_malloc() {
    TRACE("");
    GameAllocMemPointer = GameMemoryPointer;
    GameAllocMemFree = GameMemorySize;
    GameAllocMemUsed = 0;
}

void __cdecl game_free(int freeSize) {
    TRACE("");
    GameAllocMemPointer -= freeSize;
    GameAllocMemFree += freeSize;
    GameAllocMemUsed -= freeSize;
}

void __cdecl DB_Log(char *a1, ...) {
    va_list va;
    char buf[256];

    va_start(va, a1);
    if (!dword_45A1F0) {
        vsprintf(buf, a1, va);
        TRACE(buf);
        OutputDebugStringA(buf);
        OutputDebugStringA("\n");
    }
}

const char *GetFullPath(const char *filename) {
    TRACE(filename);
    #if defined FEATURE_NOCD_DATA
        sprintf(newpath, ".\\%s", filename);
    #else
        if (DEMO)
            sprintf(newpath, "%c:\\tomb\\%s", cd_drive, filename);
        else
            sprintf(newpath, "%c:\\%s", cd_drive, filename);
    #endif
    return newpath;
}

int FindCdDrive() {
    TRACE("");
    FILE *fp;
    char root[5] = "C:\\";
    char tmp_path[MAX_PATH];

    for (cd_drive = 'C'; cd_drive <= 'Z'; cd_drive ++) {
        root[0] = cd_drive;
        if (GetDriveType(root) == DRIVE_CDROM) {
            sprintf(tmp_path, "%c:\\tomb\\data\\title.phd", cd_drive);
            fp = fopen(tmp_path, "rb");
            if (fp) {
                DEMO = 1;
                return fclose(fp);
            }
            sprintf(tmp_path, "%c:\\data\\title.phd", cd_drive);
            fp = fopen(tmp_path, "rb");
            if (fp) {
                DEMO = 0;
                return fclose(fp);
            }
        }
    }
    ((void(*)(const char*))0x43D770)("ERROR: Please insert TombRaider CD");
    return 0;
}

int LoadRooms(FILE *fp) {
    TRACE("");
    uint16_t count2;
    uint32_t count4;

    _fread(&RoomCount, sizeof(uint16_t), 1, fp);
    if (RoomCount > 1024) {
        strcpy(StringToShow, "LoadRoom(): Too many rooms");
        return 0;
    }

    RoomInfo = (ROOM_INFO *)game_malloc(
        sizeof(ROOM_INFO) * RoomCount, GBUF_RoomInfos
    );
    if (!RoomInfo) {
        strcpy(StringToShow, "LoadRoom(): Could not allocate memory for rooms");
        return 0;
    }

    int i = 0;
    for (
        ROOM_INFO *current_room_info = RoomInfo;
        i < RoomCount;
        ++i, ++current_room_info
    ) {
        // Room position
        _fread(&current_room_info->x, sizeof(uint32_t), 1, fp);
        current_room_info->y = 0;
        _fread(&current_room_info->z, sizeof(uint32_t), 1, fp);

        // Room floor/ceiling
        _fread(&current_room_info->min_floor, sizeof(uint32_t), 1, fp);
        _fread(&current_room_info->max_ceiling, sizeof(uint32_t), 1, fp);

        // Room mesh
        _fread(&count4, sizeof(uint32_t), 1, fp);
        current_room_info->data = (uint16_t *)game_malloc(
            sizeof(uint16_t) * count4, GBUF_RoomMesh
        );
        _fread(current_room_info->data, sizeof(uint16_t), count4, fp);

        // Doors
        _fread(&count2, sizeof(uint16_t), 1, fp);
        if (!count2) {
            current_room_info->doors = NULL;
        } else {
            current_room_info->doors = (DOOR_INFOS *)game_malloc(
                sizeof(uint16_t) + sizeof(DOOR_INFO) * count2, GBUF_RoomDoor
            );
            current_room_info->doors->count = count2;
            _fread(
                &current_room_info->doors->door, sizeof(DOOR_INFO), count2, fp
            );
        }

        // Room floor
        _fread(&current_room_info->x_size, sizeof(uint16_t), 1, fp);
        _fread(&current_room_info->y_size, sizeof(uint16_t), 1, fp);
        count4 = current_room_info->y_size * current_room_info->x_size;
        current_room_info->floor = (FLOOR_INFO *)game_malloc(
            sizeof(FLOOR_INFO) * count4, GBUF_RoomFloor
        );
        _fread(current_room_info->floor, sizeof(FLOOR_INFO), count4, fp);

        // Room lights
        _fread(&current_room_info->ambient, sizeof(uint16_t), 1, fp);
        _fread(&current_room_info->num_lights, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_lights) {
            current_room_info->light = NULL;
        } else {
            current_room_info->light = (LIGHT_INFO *)game_malloc(
                sizeof(LIGHT_INFO) * current_room_info->num_lights,
                GBUF_RoomLights
            );
            _fread(
                current_room_info->light,
                sizeof(LIGHT_INFO),
                current_room_info->num_lights,
                fp
            );
        }

        // Static mesh infos
        _fread(&current_room_info->num_meshes, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_meshes) {
            current_room_info->mesh = NULL;
        } else {
            current_room_info->mesh = (MESH_INFO *)game_malloc(
                sizeof(MESH_INFO) * current_room_info->num_meshes,
                GBUF_RoomStaticMeshInfos
            );
            _fread(
                current_room_info->mesh,
                sizeof(MESH_INFO),
                current_room_info->num_meshes,
                fp
            );
        }

        // Flipped (alternative) room
        _fread(&current_room_info->flipped_room, sizeof(uint16_t), 1, fp);

        // Room flags
        _fread(&current_room_info->flags, sizeof(uint16_t), 1, fp);

        // Initialise some variables
        current_room_info->bound_active = 0;
        current_room_info->bound_left = PhdWinMaxX;
        current_room_info->bound_top = PhdWinMaxY;
        current_room_info->bound_bottom = 0;
        current_room_info->bound_right = 0;
        current_room_info->item_number = -1;
        current_room_info->fx_number = -1;
    }

    _fread(&count4, sizeof(uint32_t), 1, fp);
    FloorData = game_malloc(sizeof(uint16_t) * count4, GBUF_FloorData);
    _fread(FloorData, sizeof(uint16_t), count4, fp);

    return 1;
}

void __cdecl LevelStats(int levelID) {
    TRACE("");

    if (TR1MConfig.keep_health_between_levels) {
        TR1MData.stored_lara_health = LaraItem ? LaraItem->hit_points : LARA_HITPOINTS;
    }

    static char buf[100];

    TempVideoAdjust(HiRes, 1.0);
    T_InitPrint();

    sprintf(buf, "%s", LevelTitles[levelID]);
    unsigned int *txt = T_Print(0, -50, 0, buf);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    int timeTakenInSeconds = TimeTaken / 0x1Eu;
    if (timeTakenInSeconds / 3600) {
        sprintf(
            buf,
            "%s %d:%d%d:%d%d",
            "TIME TAKEN",
            timeTakenInSeconds / 3600,
            ((timeTakenInSeconds / 60) % 60) / 10,
            ((timeTakenInSeconds / 60) % 60) % 10,
            (timeTakenInSeconds % 60) / 10,
            (timeTakenInSeconds % 60) % 10
        );
    } else {
        sprintf(
            buf,
            "%s %d:%d%d",
            "TIME TAKEN",
            (timeTakenInSeconds / 60) % 60,
            (timeTakenInSeconds % 60) / 10,
            (timeTakenInSeconds % 60) % 10
        );
    }

    txt = T_Print(0, 70, 0, buf);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    int secretsTaken = 0;
    int secretsTotal = 16;
    do {
        if (SecretsTaken & 1) {
            ++secretsTaken;
        }
        SecretsTaken >>= 1;
        --secretsTotal;
    }
    while (secretsTotal);
    sprintf(
        buf,
        "%s %d %s %d",
        "SECRETS",
        secretsTaken,
        "OF",
        SecretCounts[levelID]
    );
    txt = T_Print(0, 40, 0, buf);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    sprintf(buf, "%s %d", "PICKUPS", PickupsTaken);
    txt = T_Print(0, 10, 0, buf);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    sprintf(buf, "%s %d", "KILLS", Kills);
    txt = T_Print(0, -20, 0, buf);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    while (InputStatus & 0x100000) {
        S_UpdateInput();
    }
    S_InitialisePolyList();
    S_CopyBufferToScreen();
    S_UpdateInput();
    T_DrawText();
    S_OutputPolyList();
    S_DumpScreen();
    while (!(InputStatus & 0x100000)) {
        if (IsResetFlag) {
            break;
        }
        S_InitialisePolyList();
        S_CopyBufferToScreen();
        S_UpdateInput();
        T_DrawText();
        S_OutputPolyList();
        S_DumpScreen();
    }

    if (levelID == 15) {
        Byte45BB17 = 1;
        int tmp = 1;
        do
            sub_434520(tmp++);
        while ( tmp <= 15 );
        Word45BB14 = 1;
    } else {
        CreateStartInfo(levelID + 1);
        sub_434520(levelID + 1);
        Word45BB14 = levelID + 1;
    }

    Word45BB08 &= 0xFFFEu;
    sub_41CD10();
    TempVideoRemove();
}

int __cdecl LoadLevelByID(int levelID) {
    TRACE("%d", levelID);
    int ret = LoadLevel(LevelNames[levelID], levelID);

    if (TR1MConfig.keep_health_between_levels) {
        // check if we're in main menu by seeing if there is Lara item in the
        // currently loaded level.
        int laraFound = 0;
        for (int i = 0; i < LevelItemCount; i++) {
            if (Items[i].object_id == ID_LARA) {
                laraFound = 1;
            }
        }

        if (!laraFound) {
            TR1MData.stored_lara_health = LARA_HITPOINTS;
        }
    }

    return ret;
}

int __cdecl S_DrawHealthBar(int percent) {
    TRACE("");
    int v1; // esi
    int result; // eax
    int v3; // esi

    v1 = 100 * percent / 100;
    ins_line(7, 7, 109, 7, -100, 0);
    ins_line(7, 8, 109, 8, -100, 0);
    ins_line(7, 9, 109, 9, -100, 0);
    ins_line(7, 10, 109, 10, -100, 0);
    ins_line(7, 11, 109, 11, -100, 0);
    ins_line(7, 12, 109, 12, -100, 0);
    ins_line(7, 13, 109, 13, -100, 0);
    ins_line(6, 14, 110, 14, -200, 17);
    ins_line(110, 6, 110, 14, -200, 17);
    ins_line(6, 6, 110, 6, -300, 19);
    result = ins_line(6, 6, 6, 14, -300, 19);
    if ( v1 ) {
        v3 = v1 + 8;
        ins_line(8, 8, v3, 8, -400, 8);
        ins_line(8, 9, v3, 9, -400, 11);
        ins_line(8, 10, v3, 10, -400, 8);
        ins_line(8, 11, v3, 11, -400, 6);
        result = ins_line(8, 12, v3, 12, -400, 24);
    }
    return result;
}

// int __cdecl my_ins_line(int a1, int a2, int a3, int a4, int a5, char a6) {
//     TRACE("");
//     return 0;
// }

int __cdecl LoadItems(FILE *handle)
{
    int itemCount = 0;
    _fread(&itemCount, 4u, 1u, handle);

    TRACE("Item count: %d", itemCount);

    if (itemCount) {
        if (itemCount > 256) {
            strcpy(StringToShow, "LoadItems(): Too Many Items being Loaded!!");
            return 0;
        }

        Items = game_malloc(17408, 18);
        if (!Items) {
            strcpy(
                StringToShow,
                "LoadItems(): Unable to allocate memory for 'items'"
            );
            return 0;
        }

        LevelItemCount = itemCount;
        InitialiseItemArray(256);

        for (int i = 0; i < itemCount; ++i) {
            ITEM_INFO *currentItem = &Items[i];
            _fread(&currentItem->object_id, 2u, 1u, handle);
            _fread(&currentItem->room_number, 2u, 1u, handle);
            _fread(&currentItem->pos.x, 4u, 1u, handle);
            _fread(&currentItem->pos.y, 4u, 1u, handle);
            _fread(&currentItem->pos.z, 4u, 1u, handle);
            _fread(&currentItem->pos.rot_y, 2u, 1u, handle);
            _fread(&currentItem->shade1, 2u, 1u, handle);
            _fread(&currentItem->flags, 2u, 1u, handle);

            int object_id = currentItem->object_id;
            if (object_id < 0 || object_id >= ID_NUMBER_OBJECTS) {
                sprintf(
                    StringToShow,
                    "LoadItems(): Bad Object number (%d) on Item %d",
                    object_id,
                    i
                );
                S_ExitSystem(StringToShow);
            }

            if (TR1MConfig.disable_medpacks && (
                object_id == ID_LARGE_MEDIPACK_ITEM ||
                object_id == ID_SMALL_MEDIPACK_ITEM
            )) {
                currentItem->pos.x = -1;
                currentItem->pos.y = -1;
                currentItem->pos.z = -1;
                currentItem->room_number = 0;
            }

            InitialiseItem(i);
        }
    }

    return 1;
}

void __cdecl InitialiseLara() {
    TRACE("");
    LaraItem->more_flags &= 0xFFDFu;
    LaraItem->data = &Lara;

    LaraItem->hit_points = TR1MConfig.keep_health_between_levels
        ? TR1MData.stored_lara_health
        : LARA_HITPOINTS;

    Lara.air = LARA_AIR;
    Lara.torso_z_rot = 0;
    Lara.torso_x_rot = 0;
    Lara.torso_y_rot = 0;
    Lara.head_z_rot = 0;
    Lara.head_y_rot = 0;
    Lara.head_x_rot = 0;
    Lara.calc_fallspeed = 0;
    Lara.mesh_effects = 0;
    Lara.hit_frames = 0;
    Lara.hit_direction = 0;
    Lara.death_count = 0;
    Lara.target = 0;
    Lara.spaz_effect = 0;
    Lara.spaz_effect_count = 0;
    Lara.turn_rate = 0;
    Lara.move_angle = 0;
    Lara.right_arm.flash_gun = 0;
    Lara.left_arm.flash_gun = 0;
    Lara.right_arm.lock = 0;
    Lara.left_arm.lock = 0;

    if (RoomInfo[LaraItem->room_number].flags & 1) {
        Lara.water_status = LARA_UNDERWATER;
        LaraItem->fall_speed = 0;
        LaraItem->goal_anim_state = AS_TREAD;
        LaraItem->current_anim_state = AS_TREAD;
        LaraItem->anim_number = TREAD_A;
        LaraItem->frame_number = TREAD_F;
    } else {
        Lara.water_status = LARA_ABOVEWATER;
        LaraItem->goal_anim_state = AS_STOP;
        LaraItem->current_anim_state = AS_STOP;
        LaraItem->anim_number = STOP_A;
        LaraItem->frame_number = STOP_F;
    }

    Lara.current_active = 0;

    InitialiseLOT(&Lara.LOT);
    Lara.LOT.step = WALL_L * 20;
    Lara.LOT.drop = -WALL_L * 20;
    Lara.LOT.fly = STEP_L;

    InitialiseLaraInventory(CurrentLevel);
}
