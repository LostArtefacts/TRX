#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <dbghelp.h>

#define FEATURE_NOCD_DATA
#define FEATURE_KEEP_HEALTH_BETWEEN_LEVELS
#define FEATURE_DISABLE_MEDPACKS

#ifdef FEATURE_KEEP_HEALTH_BETWEEN_LEVELS
static int TR1MStoredLaraHealth = 1000;
#endif

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
    LPVOID data;
    PHD_3DPOS pos;
    UINT16 moreFlags;
} ITEM_INFO;

typedef struct LaraInfo_t {
    __int16 itemNumber;
    __int16 gunStatus;
    __int16 field_4;
    __int16 field_6;
    __int16 field_8;
    __int16 waterStatus;
    __int16 field_C;
    __int16 field_E;
    __int16 field_10;
    __int16 air;
    __int16 field_14;
    __int16 field_16;
    __int16 field_18;
    __int16 field_1A;
    __int32 field_1C;
    __int32 field_20;
    __int8 field_24[60];
    __int32 field_60;
    __int8 field_64[4];
    __int32 field_68;
    __int32 field_6C;
    __int32 field_70;
    __int32 field_74;
    __int8 field_78[6];
    __int16 field_7E;
    __int8 field_80[6];
    __int16 field_86;
    __int8 field_88[6];
    __int16 field_8E;
    __int8 field_90[6];
    __int16 field_96;
    __int32 field_98;
    __int8 field_9C[8];
    __int32 field_A4;
    __int8 field_A8[8];
    __int32 field_B0;
    __int8 field_B4[8];
    __int32 field_BC;
    __int8 field_C0[8];
    __int8 field_C8[8];
    __int16 field_D0;
    __int16 field_D2;
    __int16 field_D4;
    __int16 field_D6;
    __int16 field_D8;
} LARA_INFO;

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


static void InjectFunc(void *from, void *to) {
    DWORD tmp;
    TRACE("Patching %p to %p", from, to);
    VirtualProtect(from, sizeof(JMP), PAGE_EXECUTE_READWRITE, &tmp);
    HANDLE hCurrentProcess = GetCurrentProcess();
    JMP buf;
    buf.opCode = 0xE9;
    buf.offset = (DWORD)(to) - ((DWORD)(from) + sizeof(JMP));
    WriteProcessMemory(hCurrentProcess, from, &buf, sizeof(JMP), &tmp);
    CloseHandle(hCurrentProcess);
    // arsunt style
    //((JMP*)(from))->opCode = 0xE9;
    //((JMP*)(from))->offset = (DWORD)(to) - ((DWORD)(from) + sizeof(JMP));
}

static void PrintStackTrace() {
    const size_t MaxNameLen = 255;
    BOOL result;
    HANDLE process;
    HANDLE thread;
    CONTEXT context;
    STACKFRAME64 stack;
    ULONG frame;
    DWORD64 displacement;
    IMAGEHLP_SYMBOL64 *pSymbol = malloc(
        sizeof(IMAGEHLP_SYMBOL64) + (MaxNameLen + 1) * sizeof(TCHAR)
    );
    char *name = malloc(MaxNameLen + 1);

    RtlCaptureContext(&context);
    memset(&stack, 0, sizeof(STACKFRAME64));

    process = GetCurrentProcess();
    thread = GetCurrentThread();
    displacement = 0;
    stack.AddrPC.Offset = context.Eip;
    stack.AddrPC.Mode = AddrModeFlat;
    stack.AddrStack.Offset = context.Esp;
    stack.AddrStack.Mode = AddrModeFlat;
    stack.AddrFrame.Offset = context.Ebp;
    stack.AddrFrame.Mode = AddrModeFlat;

    for (frame = 0; ; frame++) {
        result = StackWalk64(
             IMAGE_FILE_MACHINE_I386,
             process,
             thread,
             &stack,
             &context,
             NULL,
             SymFunctionTableAccess64,
             SymGetModuleBase64,
             NULL
         );

        pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        pSymbol->MaxNameLength = MaxNameLen;

        SymGetSymFromAddr64(
            process, (ULONG64)stack.AddrPC.Offset, &displacement, pSymbol
        );
        UnDecorateSymbolName(
            pSymbol->Name, ( PSTR )name, MaxNameLen, UNDNAME_COMPLETE
        );

        TRACE(
             "Frame %lu:\n"
             "    Symbol name:    %s\n"
             "    PC address:     0x%08LX\n"
             "    Stack address:  0x%08LX\n"
             "    Frame address:  0x%08LX\n"
             "\n",
             frame,
             pSymbol->Name,
             (ULONG64)stack.AddrPC.Offset,
             (ULONG64)stack.AddrStack.Offset,
             (ULONG64)stack.AddrFrame.Offset
         );

        if (!result) {
            break;
        }
    }
    free(pSymbol);
    free(name);
}

#define INJECT(from, to) { \
    InjectFunc((void*)from, (void*)to); \
}

// data
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
#define CurrentLevel            VAR_U_(0x00453C4C, __int32)
#define Lara                    VAR_U_(0x0045ED80, LARA_INFO)
#define LaraItem                VAR_U_(0x0045EE6C, ITEM_INFO*)
#define LevelNames              ARRAY_(0x00453648, const char*, [22])
#define LevelTitles             ARRAY_(0x00453DF8, const char*, [22])
#define SecretCounts            ARRAY_(0x00453CB0, __int8, [16])
#define IsResetFlag             VAR_U_(0x00459F50, int)
#define Word45BB14              VAR_U_(0x0045BB14, __int16)
#define Byte45BB17              VAR_U_(0x0045BB17, __int8)
#define Word45BB08              VAR_U_(0x0045BB08, __int16)
#define InputStatus             VAR_U_(0x0045EEF4, int)
#define Kills                   VAR_U_(0x0045BB0E, int)
#define TimeTaken               VAR_U_(0x0045BB0A, int)
#define PickupsTaken            VAR_U_(0x0045BB16, int)
#define SecretsTaken            VAR_U_(0x0045BB12, int)
#define HiRes                   VAR_U_(0x00459F64, int)

// functions
#define game_malloc             ((void      __cdecl*(*)(int, int))0x0041E2F0)
#define ins_line                ((int       __cdecl(*)(int, int, int, int, int, char))0x00402710)
#define _fread                  ((size_t    __cdecl(*)(void *, size_t, size_t, FILE *))0x00442C20)
#define InitialiseItemArray     ((void      __cdecl(*)(int itemCount))0x00421B10)
#define S_ExitSystem            ((void      __cdecl(*)(const char *))0x0041E260)
#define InitialiseItem          ((void      __cdecl(*)(__int16))0x00421CC0)
#define InitialiseLaraInventory ((void      __cdecl(*)(int levelID))0x00428170)
#define InitialiseLOT           ((void      __cdecl(*)())0x0042A780)
#define LoadLevel               ((int       __cdecl(*)(const char *path, int levelID))0x0041AFB0)
#define T_DrawText              ((void      __cdecl(*)())0x00439B00)
#define S_DumpScreen            ((__int32   __cdecl(*)())0x0042FC70)
#define S_InitialisePolyList    ((void      __cdecl(*)())0x0042FC60)
#define S_UpdateInput           ((void      __cdecl(*)())0x0041E550)
#define S_CopyBufferToScreen    ((void      __cdecl(*)())0x00416A60)
#define S_OutputPolyList        ((void      __cdecl(*)())0x0042FD10)
#define sub_41CD10              ((void      __cdecl(*)())0x0041CD10)
#define sub_434520              ((void      __cdecl(*)(int))0x00434520)
#define CreateStartInfo         ((void      __cdecl(*)(int levelID))0x004345E0)
#define TempVideoRemove         ((void      __cdecl(*)())0x004167D0)
#define T_InitPrint             ((void      __cdecl(*)())0x00439750)
#define T_Print                 ((int*      __cdecl(*)(__int16, __int16, __int16, const char *))0x00439780)
#define T_CentreH               ((unsigned int* __cdecl(*)(unsigned int*, __int16))0x004399A0)
#define T_CentreV               ((unsigned int* __cdecl(*)(unsigned int*, __int16))0x004399C0)
#define TempVideoAdjust         ((void      __cdecl(*)(int hi_res, double sizer))0x00416550)

// enums
#define ID_LARA 0
#define ID_SMALL_MEDIPACK_ITEM 93
#define ID_LARGE_MEDIPACK_ITEM 94
#define ID_NUMBER_OBJECTS 191

// function reimplementations
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

static void Inject() {
    INJECT(0x0042A2C0, DB_Log);
    INJECT(0x0041C020, FindCdDrive);
    INJECT(0x0041BFC0, GetFullPath);
    INJECT(0x0041B3F0, LoadRooms);
    INJECT(0x0041E2C0, init_game_malloc);
    INJECT(0x0041E3B0, game_free);

    INJECT(0x0041BC60, LoadItems);
    INJECT(0x00428020, InitialiseLara);
    INJECT(0x0041AF90, LoadLevelByID);
    INJECT(0x0041D5A0, LevelStats);

    //INJECT(0x00402710, my_ins_line);

    //INJECT(0x004302D0, S_DrawHealthBar);
    //INJECT(0x00430450, S_DrawAirBar);
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
