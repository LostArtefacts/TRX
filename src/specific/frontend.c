#include "specific/frontend.h"

#include "game/const.h"
#include "game/vars.h"
#include "specific/display.h"
#include "specific/init.h"
#include "specific/shed.h"

#include "config.h"
#include "util.h"

#include <stdlib.h>

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

    return ret;
}

void T1MInjectSpecificFrontend()
{
    INJECT(0x0041D040, S_PlayFMV);
}
