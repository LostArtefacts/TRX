#include "specific/s_init.h"

#include "3dsystem/phd_math.h"
#include "game/clock.h"
#include "game/game.h"
#include "game/gamebuf.h"
#include "game/music.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/lib.h"
#include "global/vars.h"
#include "specific/s_display.h"
#include "specific/s_file.h"
#include "specific/s_frontend.h"
#include "specific/s_hwr.h"
#include "specific/s_input.h"
#include "specific/s_main.h"

#include <stdio.h>
#include <time.h>
#include <windows.h>

void S_InitialiseSystem()
{
    S_SeedRandom();

    Lib_Init();
    Text_Init();
    Clock_Init();
    SoundIsActive = Sound_Init();
    Music_Init();
    InputInit();
    FMVInit();

    CalculateWibbleTable();

    HWR_InitialiseHardware();
}

void S_ExitSystem(const char *message)
{
    while (Input.select) {
        S_UpdateInput();
    }
    GameBuf_Shutdown();
    HWR_ShutdownHardware();
    ShowFatalError(message);
}

void S_ExitSystemFmt(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    char message[150];
    vsnprintf(message, 150, fmt, va);
    va_end(va);
    S_ExitSystem(message);
}

void CalculateWibbleTable()
{
    for (int i = 0; i < WIBBLE_SIZE; i++) {
        PHD_ANGLE angle = (i * PHD_360) / WIBBLE_SIZE;
        WibbleTable[i] = phd_sin(angle) * MAX_WIBBLE >> W2V_SHIFT;
        ShadeTable[i] = phd_sin(angle) * MAX_SHADE >> W2V_SHIFT;
        RandTable[i] = (GetRandomDraw() >> 5) - 0x01FF;
    }
}

void S_SeedRandom()
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);
    SeedRandomControl(tptr->tm_sec + 57 * tptr->tm_min + 3543 * tptr->tm_hour);
    SeedRandomDraw(tptr->tm_sec + 43 * tptr->tm_min + 3477 * tptr->tm_hour);
}
