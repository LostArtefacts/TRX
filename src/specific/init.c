#include "3dsystem/phd_math.h"
#include "game/vars.h"
#include "game/game.h"
#include "specific/file.h"
#include "specific/init.h"
#include "specific/shed.h"
#include "specific/sndpc.h"
#include "util.h"
#include <stdarg.h>
#include <time.h>
#include <windows.h>

void DB_Log(char* fmt, ...)
{
    va_list va;
    char buffer[256] = { 0 };

    va_start(va, fmt);
    if (!dword_45A1F0) {
        vsprintf(buffer, fmt, va);
        TRACE(buffer);
        OutputDebugStringA(buffer);
        OutputDebugStringA("\n");
    }
}

void S_InitialiseSystem()
{
    TRACE("");
    FindCdDrive();

    GameVidWidth = 640;
    GameVidHeight = 480;

    DumpX = 0;
    DumpY = 0;
    DumpWidth = 640;
    DumpHeight = 480;

    sub_4380E0(&GameVidWidth);

    SoundStart();
    if (SoundInit() == -1) {
        SoundIsActive = 0;
    }

    CalculateWibbleTable();

#ifdef T1M_FEAT_EXTENDED_MEMORY
    // TODO: for this to work, the value also needs to be updated in
    // GameMain and S_PlayFMV. Since exceeding this limit causes the game
    // to brutally crash anyway, I'm leaving it unfinished here.
    GameMemorySize = 0x1000000;
#else
    GameMemorySize = 0x380000;
#endif

    InitialiseHardware();
}

void init_game_malloc()
{
    TRACE("");
    GameAllocMemPointer = GameMemoryPointer;
    GameAllocMemFree = GameMemorySize;
    GameAllocMemUsed = 0;
}

void game_free(int free_size)
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
    struct tm* tptr = localtime(&lt);
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
