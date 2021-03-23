#include "specific/init.h"

#include "3dsystem/phd_math.h"
#include "game/game.h"
#include "global/vars.h"
#include "specific/file.h"
#include "specific/frontend.h"
#include "specific/input.h"
#include "specific/shed.h"
#include "specific/smain.h"
#include "specific/sndpc.h"
#include "util.h"

#include <stdarg.h>
#include <time.h>
#include <windows.h>

void DB_Log(const char *fmt, ...)
{
    va_list va;
    char buf[256] = { 0 };

    va_start(va, fmt);
    if (!dword_45A1F0) {
        vsprintf(buf, fmt, va);
        TRACE(buf);
        OutputDebugStringA(buf);
        OutputDebugStringA("\n");
    }
}

void S_InitialiseSystem()
{
    TRACE("");
    S_SeedRandom();

    GameVidWidth = 640;
    GameVidHeight = 480;

    DumpX = 0;
    DumpY = 0;
    DumpWidth = 640;
    DumpHeight = 480;

    SWRInit();
    SoundInit();
    MusicInit();
    InputInit();
    FMVInit();

    if (!SoundInit1) {
        SoundIsActive = 0;
    }

    CalculateWibbleTable();

    GameMemorySize = MALLOC_SIZE;

    InitialiseHardware();
}

void S_ExitSystem(const char *message)
{
    while (Input & IN_SELECT) {
        S_UpdateInput();
        WinVidSpinMessageLoop();
    }
    if (GameMemoryPointer) {
        free(GameMemoryPointer);
    }
    ShutdownHardware();
    TerminateGameWithMsg(
        "\n\nTomb Raider (c) Core Design. %s %s \n%s\n%s\n", "Jan  7 1998",
        "14:53:25", " ", message);
}

void init_game_malloc()
{
    TRACE("");
    GameAllocMemPointer = GameMemoryPointer;
    GameAllocMemFree = GameMemorySize;
    GameAllocMemUsed = 0;
}

void game_free(int32_t free_size, int32_t type)
{
    TRACE("");
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
    INJECT(0x0042A2C0, DB_Log);
}
