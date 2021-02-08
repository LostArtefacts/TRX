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
    __int16 wCount;
    __int32 dwCount;

    _fread(&RoomCount, sizeof(__int16), 1, fp);
    if (RoomCount > 1024) {
        strcpy(StringToShow, "LoadRoom(): Too many rooms");
        return 0;
    }

    RoomInfo = (ROOM_INFO *)game_malloc(sizeof(ROOM_INFO) * RoomCount, GBUF_RoomInfos);
    if (!RoomInfo) {
        strcpy(StringToShow, "LoadRoom(): Could not allocate memory for rooms");
        return 0;
    }

    int i = 0;
    for (ROOM_INFO *current_room_info = RoomInfo; i < RoomCount; ++i, ++current_room_info) {
        // Room position
        _fread(&current_room_info->x, sizeof(__int32), 1, fp);
        current_room_info->y = 0;
        _fread(&current_room_info->z, sizeof(__int32), 1, fp);

        // Room floor/ceiling
        _fread(&current_room_info->minFloor, sizeof(__int32), 1, fp);
        _fread(&current_room_info->maxCeiling, sizeof(__int32), 1, fp);

        // Room mesh
        _fread(&dwCount, sizeof(__int32), 1, fp);
        current_room_info->data = (__int16 *)game_malloc(sizeof(__int16) * dwCount, GBUF_RoomMesh);
        _fread(current_room_info->data, sizeof(__int16), dwCount, fp);

        // Doors
        _fread(&wCount, sizeof(__int16), 1, fp);
        if (!wCount) {
            current_room_info->doors = NULL;
        } else {
            current_room_info->doors = (DOOR_INFOS *)game_malloc(sizeof(__int16) + sizeof(DOOR_INFO) * wCount, GBUF_RoomDoor);
            current_room_info->doors->count = wCount;
            _fread(&current_room_info->doors->door, sizeof(DOOR_INFO), wCount, fp);
        }

        // Room floor
        _fread(&current_room_info->xSize, sizeof(__int16), 1, fp);
        _fread(&current_room_info->ySize, sizeof(__int16), 1, fp);
        dwCount = current_room_info->ySize * current_room_info->xSize;
        current_room_info->floor = (FLOOR_INFO *)game_malloc(sizeof(FLOOR_INFO) * dwCount, GBUF_RoomFloor);
        _fread(current_room_info->floor, sizeof(FLOOR_INFO), dwCount, fp);

        // Room lights
        _fread(&current_room_info->ambient, sizeof(__int16), 1, fp);
        _fread(&current_room_info->numLights, sizeof(__int16), 1, fp);
        if (!current_room_info->numLights) {
            current_room_info->light = NULL;
        } else {
            current_room_info->light = (LIGHT_INFO *)game_malloc(sizeof(LIGHT_INFO) * current_room_info->numLights, GBUF_RoomLights);
            _fread(current_room_info->light, sizeof(LIGHT_INFO), current_room_info->numLights, fp);
        }

        // Static mesh infos
        _fread(&current_room_info->numMeshes, sizeof(__int16), 1, fp);
        if (!current_room_info->numMeshes) {
            current_room_info->mesh = NULL;
        } else {
            current_room_info->mesh = (MESH_INFO *)game_malloc(sizeof(MESH_INFO) * current_room_info->numMeshes, GBUF_RoomStaticMeshInfos);
            _fread(current_room_info->mesh, sizeof(MESH_INFO), current_room_info->numMeshes, fp);
        }

        // Flipped (alternative) room
        _fread(&current_room_info->flippedRoom, sizeof(__int16), 1, fp);

        // Room flags
        _fread(&current_room_info->flags, sizeof(__int16), 1, fp);

        // Initialise some variables
        current_room_info->boundActive = 0;
        current_room_info->boundLeft = PhdWinMaxX;
        current_room_info->boundTop = PhdWinMaxY;
        current_room_info->boundBottom = 0;
        current_room_info->boundRight = 0;
        current_room_info->itemNumber = -1;
        current_room_info->fxNumber = -1;
    }

    _fread(&dwCount, sizeof(__int32), 1, fp);
    FloorData = game_malloc(sizeof(__int16) * dwCount, GBUF_FloorData);
    _fread(FloorData, sizeof(__int16), dwCount, fp);

    return 1;
}

void __cdecl LevelStats(int levelID) {
    TRACE("");

    #ifdef FEATURE_KEEP_HEALTH_BETWEEN_LEVELS
        // store Lara health
        TR1MStoredLaraHealth = LaraItem ? LaraItem->hitPoints : 1000;
        TRACE("Lara pointers: %p/%d", LaraItem, Lara.itemNumber);
        TRACE("Storing Lara health: %d", LaraItem ? LaraItem->hitPoints : -1);
    #endif

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
    sprintf(buf, "%s %d %s %d", "SECRETS", secretsTaken, "OF", SecretCounts[levelID]);
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

    #ifdef FEATURE_KEEP_HEALTH_BETWEEN_LEVELS
        // check if we're in main menu by seeing if there is Lara item in the
        // currently loaded level.
        int laraFound = 0;
        for (int i = 0; i < LevelItemCount; i++) {
            if (Items[i].objectID == ID_LARA) {
                laraFound = 1;
            }
        }

        if (!laraFound) {
            TRACE("Resetting stored Lara health");
            TR1MStoredLaraHealth = 1000;
        }
    #endif

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
            strcpy(StringToShow, "LoadItems(): Unable to allocate memory for 'items'");
            return 0;
        }

        LevelItemCount = itemCount;
        InitialiseItemArray(256);

        for (int i = 0; i < itemCount; ++i) {
            ITEM_INFO *currentItem = &Items[i];
            _fread(&currentItem->objectID, 2u, 1u, handle);
            _fread(&currentItem->roomNumber, 2u, 1u, handle);
            _fread(&currentItem->pos.x, 4u, 1u, handle);
            _fread(&currentItem->pos.y, 4u, 1u, handle);
            _fread(&currentItem->pos.z, 4u, 1u, handle);
            _fread(&currentItem->pos.rotY, 2u, 1u, handle);
            _fread(&currentItem->shade1, 2u, 1u, handle);
            _fread(&currentItem->flags, 2u, 1u, handle);

            int objectID = currentItem->objectID;
            if (objectID < 0 || objectID >= ID_NUMBER_OBJECTS) {
                sprintf(StringToShow, "LoadItems(): Bad Object number (%d) on Item %d", objectID, i);
                S_ExitSystem(StringToShow);
            }

            #ifdef FEATURE_DISABLE_MEDPACKS
                if (objectID == ID_LARGE_MEDIPACK_ITEM || objectID == ID_SMALL_MEDIPACK_ITEM) {
                    currentItem->pos.x = -1;
                    currentItem->pos.y = -1;
                    currentItem->pos.z = -1;
                    currentItem->roomNumber = 0;
                }
            #endif

            InitialiseItem(i);
        }
    }

    return 1;
}

void __cdecl InitialiseLara(void) {
    TRACE("");
    LaraItem->moreFlags &= 0xFFDFu;
    LaraItem->data = &Lara;
    #ifdef FEATURE_KEEP_HEALTH_BETWEEN_LEVELS
        TRACE("Restoring Lara health: %d", TR1MStoredLaraHealth);
        LaraItem->hitPoints = TR1MStoredLaraHealth;
    #else
        TRACE("Restoring Lara health: default");
        LaraItem->hitPoints = 1000;
    #endif

    Lara.air = 1800;
    Lara.field_8 = 0;
    Lara.field_20 = 0;
    Lara.field_E = 0;
    Lara.field_10 = 0;
    Lara.field_16 = 0;
    Lara.field_60 = 0;
    Lara.field_1C = 0;
    Lara.field_1A = 0;
    Lara.field_96 = 0;
    Lara.field_86 = 0;
    Lara.field_8E = 0;
    Lara.field_7E = 0;
    Lara.field_6C = 0;
    Lara.field_68 = 0;
    Lara.field_74 = 0;
    Lara.field_70 = 0;

    TRACE("%x\n", &Lara.field_4 - &Lara.itemNumber);

    if (RoomInfo[LaraItem->roomNumber].flags & 1) {
        Lara.waterStatus = 1;
        LaraItem->fallSpeed = 0;
        LaraItem->goalAnimState = 13;
        LaraItem->currentAnimState = 13;
        LaraItem->animNumber = 108;
        LaraItem->frameNumber = 1736;
    } else {
        Lara.waterStatus = 0;
        LaraItem->goalAnimState = 2;
        LaraItem->currentAnimState = 2;
        LaraItem->animNumber = 11;
        LaraItem->frameNumber = 185;
    }

    Lara.field_18 = 0;
    InitialiseLOT(&Lara.field_C8);
    Lara.field_D4 = 20480;
    Lara.field_D6 = -20480;
    Lara.field_D8 = 256;
    InitialiseLaraInventory(CurrentLevel);
}
