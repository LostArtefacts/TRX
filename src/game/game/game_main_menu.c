#include "game/phase/phase.h"
#include "global/types.h"

#include <stdint.h>

GAMEFLOW_OPTION Game_MainMenu(void)
{
    Phase_Set(PHASE_INVENTORY, (void *)(intptr_t)INV_TITLE_MODE);
    return Phase_Run();
}
