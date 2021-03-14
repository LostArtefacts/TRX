#include "specific/frontend.h"

#include "game/const.h"
#include "game/vars.h"
#include "specific/display.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/shed.h"

#include "config.h"
#include "util.h"

#include <stdlib.h>

void S_Wait(int32_t nframes)
{
    while (Input) {
        S_UpdateInput();
    }
    for (int i = 0; i < nframes; i++) {
        S_UpdateInput();
        if (KeyData->keys_held) {
            break;
        }
        while (!WinVidSpinMessageLoop())
            ;
    }
}

int32_t S_PlayFMV(int32_t sequence, int32_t mode)
{
    if (T1MConfig.disable_fmv) { // T1M
        return -1;
    }

    if (GameMemoryPointer) {
        free(GameMemoryPointer);
    }

    TempVideoAdjust(2, 1.0);
    HardwarePrepareFMV();

    int32_t ret = WinPlayFMV(sequence, mode);

    GameMemoryPointer = malloc(MALLOC_SIZE);
    if (!GameMemoryPointer) {
        S_ExitSystem("ERROR: Could not allocate enough memory");
        return -1;
    }
    init_game_malloc();

    if (IsHardwareRenderer) {
        HardwareFMVDone();
    }
    TempVideoRemove();

    if (T1MConfig.fix_fmv_esc_key) {
        while (KeyData->keys_held) {
            S_UpdateInput();
            WinVidSpinMessageLoop();
        }
    }

    return ret;
}

void T1MInjectSpecificFrontend()
{
    INJECT(0x0041CD50, S_Wait);
    INJECT(0x0041D040, S_PlayFMV);
}
