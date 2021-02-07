#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

#define FEATURE_NOCD_DATA

typedef enum {
    GBUF_RoomInfos = 11,
    GBUF_RoomMesh = 12,
    GBUF_RoomDoor = 13,
    GBUF_RoomFloor = 14,
    GBUF_RoomLights = 15,
    GBUF_RoomStaticMeshInfos = 16,
    GBUF_FloorData = 17,
} GAMEALLOC_BUFFER;

#pragma pack(push, 1)

typedef struct Pos2D_t {
    __int16 x;
    __int16 y;
} POS_2D;

typedef struct Pos3D_t {
    __int16 x;
    __int16 y;
    __int16 z;
} POS_3D;

typedef struct DoorInfo_t {
    __int16 room;
    __int16 x;
    __int16 y;
    __int16 z;
    POS_3D vertex[4];
} DOOR_INFO;

typedef struct DoorInfos_t {
    __int16 count;
    DOOR_INFO door[];
} DOOR_INFOS;

typedef struct FloorInfo_t {
    __int16 index;
    __int16 box;
    __int8 pitRoom;
    __int8 floor;
    __int8 skyRoom;
    __int8 ceiling;
} FLOOR_INFO;

typedef struct LightInfo_t {
    __int32 x;
    __int32 y;
    __int32 z;
    __int16 intensity;
    __int32 falloff;
} LIGHT_INFO;

typedef struct MeshInfo_t {
    __int32 x;
    __int32 y;
    __int32 z;
    __int16 yRot;
    __int16 shade;
    __int16 staticNumber;
} MESH_INFO;

typedef struct RoomInfo_t {
    __int16 *data;
    DOOR_INFOS *doors;
    FLOOR_INFO *floor;
    LIGHT_INFO *light;
    MESH_INFO *mesh;
    __int32 x;
    __int32 y;
    __int32 z;
    __int32 minFloor;
    __int32 maxCeiling;
    __int16 xSize;
    __int16 ySize;
    __int16 ambient;
    __int16 numLights;
    __int16 numMeshes;
    __int16 boundLeft;
    __int16 boundRight;
    __int16 boundTop;
    __int16 boundBottom;
    __int16 boundActive;
    __int16 itemNumber;
    __int16 fxNumber;
    __int16 flippedRoom;
    __int16 flags;
} ROOM_INFO;

typedef struct Phd3dPos_t {
    int x;
    int y;
    int z;
    __int16 rotX;
    __int16 rotY;
    __int16 rotZ;
} PHD_3DPOS;

typedef struct ItemInfo_t {
    int floor;
    DWORD touchBits;
    DWORD meshBits;
    __int16 objectID;
    __int16 currentAnimState;
    __int16 goalAnimState;
    __int16 requiredAnimState;
    __int16 animNumber;
    __int16 frameNumber;
    __int16 roomNumber;
    __int16 nextItem;
    __int16 nextActive;
    __int16 speed;
    __int16 fallSpeed;
    __int16 hitPoints;
    __int16 boxNumber;
    __int16 timer;
    UINT16 flags; // see IFL_* defines
    __int16 shade1;
    //__int16 shade2;
    //__int16 carriedItem;
    LPVOID data;
    PHD_3DPOS pos;
    UINT16 moreFlags;
} ITEM_INFO;

#pragma pop

HINSTANCE hInstance = NULL;

#pragma pack(push, 1)
typedef struct {
    BYTE opCode;    // must be 0xE9;
    DWORD offset;   // jump offset
} JMP;
#pragma pack(pop)

#define TRACE(...) { \
    printf("%s:%d %s ", __FILE__, __LINE__, __func__); \
    printf(__VA_ARGS__); \
    printf("\n"); \
    fflush(stdout); \
}

#define VAR_U_(address, type)           (*(type*)(address))
#define VAR_I_(address, type, value)    (*(type*)(address))
#define ARRAY_(address, type, length)   (*(type(*)length)(address))


#define INJECT(from,to) { \
    ((JMP*)(from))->opCode = 0xE9; \
    ((JMP*)(from))->offset = (DWORD)(to) - ((DWORD)(from) + sizeof(JMP)); \
}

#define cd_drive                VAR_I_(0x0045A010, char, '.')
#define DEMO                    VAR_I_(0x0045F1C0, __int32, 0)
#define dword_45A1F0            VAR_U_(0x0045A1F0, __int32)
#define newpath                 ARRAY_(0x00459F90, char, [128])
#define RoomCount               VAR_U_(0x00462BDC, __int16)
#define RoomInfo                VAR_U_(0x00462BE8, ROOM_INFO*)
#define PhdWinMaxX              VAR_I_(0x006CAD00, __int32, 0)
#define PhdWinMaxY              VAR_I_(0x006CAD10, __int32, 0)
#define Meshes                  VAR_U_(0x0045F1B8, __int16*)
#define FloorData               VAR_U_(0x0045F1BC, __int16*)
#define StringToShow            ARRAY_(0x00456AD0, char, [128])
#define MeshPtr                 VAR_U_(0x00461F34, __int16**)
#define LevelItemCount          VAR_U_(0x0045A0E0, __int32)
#define Items                   VAR_U_(0x00462CEC, ITEM_INFO*)
#define GameAllocMemPointer     VAR_U_(0x0045E32C, __int32)
#define GameAllocMemUsed        VAR_U_(0x0045E330, __int32)
#define GameAllocMemFree        VAR_U_(0x0045E334, __int32)
#define GameMemoryPointer       VAR_U_(0x0045A034, __int32)
#define GameMemorySize          VAR_U_(0x0045EEF8, __int32)

// foreign game functions
#define game_malloc     ((void __cdecl*(*)(int, int))0x0041E2F0)
#define _fread          ((size_t __cdecl(*)(void *a1, size_t a2, size_t a3, FILE *a4))0x00442C20)
#define InitialiseItemArray ((void __cdecl(*)(int itemCount))0x00421B10)
#define S_ExitSystem ((void __cdecl(*)(const char *))0x0041E260)
#define InitialiseItem ((void __cdecl(*)(__int16))0x00421CC0)

void __cdecl init_game_malloc() {
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

void DB_Log(char *a1, ...) {
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
            TRACE("Items[%d].objectID: %d", i, currentItem->objectID);
            int objectID = currentItem->objectID;
            if ( objectID < 0 || objectID >= 191 )
            {
                sprintf(StringToShow, "LoadItems(): Bad Object number (%d) on Item %d", objectID, i);
                S_ExitSystem(StringToShow);
            }
            InitialiseItem(i);
        }
    }

    return 1;
}

static void Inject() {
    INJECT(0x0042A2C0, DB_Log);
    INJECT(0x0041C020, FindCdDrive);
    INJECT(0x0041BFC0, GetFullPath);
    INJECT(0x0041B3F0, LoadRooms);
    INJECT(0x0041E2C0, init_game_malloc);
    INJECT(0x0041E3B0, game_free);

    INJECT(0x0041BC60, LoadItems);
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            freopen("./TR1Main.log", "w", stdout);
            TRACE("Attached");
            hInstance = hinstDLL;
            Inject();
            break;

        case DLL_PROCESS_DETACH:
            TRACE("Detached");
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}
