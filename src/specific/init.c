#include "specific/init.h"

#include "3dsystem/phd_math.h"
#include "game/game.h"
#include "global/vars.h"
#include "specific/clock.h"
#include "specific/file.h"
#include "specific/frontend.h"
#include "specific/hwr.h"
#include "specific/input.h"
#include "specific/shed.h"
#include "specific/smain.h"
#include "specific/sndpc.h"
#include "util.h"

#include <stdarg.h>
#include <time.h>
#include <windows.h>

static const char *BufferNames[] = {
    "Sprite Textures", // GBUF_TEXTURE_PAGES
    "Object Textures", // GBUF_OBJECT_TEXTURES
    "Mesh Pointers", // GBUF_MESH_POINTERS
    "Meshes", // GBUF_MESHES
    "Anims", // GBUF_ANIMS
    "Structs", // GBUF_ANIM_CHANGES
    "Ranges", // GBUF_ANIM_RANGES
    "Commands", // GBUF_ANIM_COMMANDS
    "Bones", // GBUF_ANIM_BONES
    "Frames", // GBUF_ANIM_FRAMES
    "Room Textures", // GBUF_ROOM_TEXTURES
    "Room Infos", // GBUF_ROOM_INFOS
    "Room Mesh", // GBUF_ROOM_MESH
    "Room Door", // GBUF_ROOM_DOOR
    "Room Floor", // GBUF_ROOM_FLOOR
    "Room Lights", // GBUF_ROOM_LIGHTS
    "Room Static Mesh Infos", // GBUF_ROOM_STATIC_MESH_INFOS
    "Floor Data", // GBUF_FLOOR_DATA
    "ITEMS!!", // GBUF_ITEMS
    "Cameras", // GBUF_CAMERAS
    "Sound FX", // GBUF_SOUND_FX
    "Boxes", // GBUF_BOXES
    "Overlaps", // GBUF_OVERLAPS
    "GroundZone", // GBUF_GROUNDZONE
    "FlyZone", // GBUF_FLYZONE
    "Animating Texture Ranges", // GBUF_ANIMATING_TEXTURE_RANGES
    "Cinematic Frames", // GBUF_CINEMATIC_FRAMES
    "LoadDemo Buffer", // GBUF_LOADDEMO_BUFFER
    "SaveDemo Buffer", // GBUF_SAVEDEMO_BUFFER
    "Cinematic Effects", // GBUF_CINEMATIC_EFFECTS
    "Mummy Head Turn", // GBUF_MUMMY_HEAD_TURN
    "Extra Door stuff", // GBUF_EXTRA_DOOR_STUFF
    "Effects_Array", // GBUF_EFFECTS
    "Creature Data", // GBUF_CREATURE_DATA
    "Creature LOT", // GBUF_CREATURE_LOT
    "Sample Infos", // GBUF_SAMPLE_INFOS
    "Samples", // GBUF_SAMPLES
    "Sample Offsets", // GBUF_SAMPLE_OFFSETS
    "Rolling Ball Stuff", // GBUF_ROLLINGBALL_STUFF
};

void DB_Log(const char *fmt, ...)
{
    va_list va;
    char buf[256] = { 0 };

    va_start(va, fmt);
    vsprintf(buf, fmt, va);
    LOG_INFO("%s", buf);
    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
}

void S_InitialiseSystem()
{
    S_SeedRandom();

    GameVidWidth = 640;
    GameVidHeight = 480;

    DumpX = 0;
    DumpY = 0;
    DumpWidth = 640;
    DumpHeight = 480;

    SWRInit();
    ClockInit();
    SoundInit();
    MusicInit();
    InputInit();
    FMVInit();

    if (!SoundInit1) {
        SoundIsActive = 0;
    }

    CalculateWibbleTable();

    GameMemorySize = MALLOC_SIZE;

    HWR_InitialiseHardware();
}

void S_ExitSystem(const char *message)
{
    while (Input & IN_SELECT) {
        S_UpdateInput();
    }
    if (GameMemoryPointer) {
        free(GameMemoryPointer);
    }
    HWR_ShutdownHardware();
    ShowFatalError(message);
}

void init_game_malloc()
{
    LOG_DEBUG("");
    GameAllocMemPointer = GameMemoryPointer;
    GameAllocMemFree = GameMemorySize;
    GameAllocMemUsed = 0;
}

void *game_malloc(int32_t alloc_size, GAMEALLOC_BUFFER buf_index)
{
    int32_t aligned_size;

    aligned_size = (alloc_size + 3) & ~3;

    if (aligned_size > GameAllocMemFree) {
        sprintf(
            StringToShow, "game_malloc(): OUT OF MEMORY %s %d",
            BufferNames[buf_index], aligned_size);
        S_ExitSystem(StringToShow);
    }

    void *result = GameAllocMemPointer;
    GameAllocMemFree -= aligned_size;
    GameAllocMemUsed += aligned_size;
    GameAllocMemPointer += aligned_size;
    return result;
}

void game_free(int32_t free_size, int32_t type)
{
    LOG_DEBUG("");
    GameAllocMemPointer -= free_size;
    GameAllocMemFree += free_size;
    GameAllocMemUsed -= free_size;
}

void CalculateWibbleTable()
{
    for (int i = 0; i < WIBBLE_SIZE; i++) {
        PHD_ANGLE angle = (i * 65536) / WIBBLE_SIZE;
        WibbleTable[i] = phd_sin(angle) * MAX_WIBBLE >> W2V_SHIFT;
        ShadeTable[i] = phd_sin(angle) * MAX_SHADE >> W2V_SHIFT;
        RandTable[i] = (GetRandomDraw() >> 5) - 0x01ff;
    }
}

void S_SeedRandom()
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);
    SeedRandomControl(tptr->tm_sec + 57 * tptr->tm_min + 3543 * tptr->tm_hour);
    SeedRandomDraw(tptr->tm_sec + 43 * tptr->tm_min + 3477 * tptr->tm_hour);
}

void T1MInjectSpecificInit()
{
    INJECT(0x0041E100, S_InitialiseSystem);
    INJECT(0x0041E2C0, init_game_malloc);
    INJECT(0x0041E3B0, game_free);

    // va_args causes crashes on certain platforms
    // INJECT(0x0042A2C0, DB_Log);
}
