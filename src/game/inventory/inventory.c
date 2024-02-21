#include "game/phase/phase.h"
#include "global/types.h"

#include <stdint.h>

GAMEFLOW_OPTION Inv_Display(const INV_MODE inv_mode)
{
    if (inv_mode == INV_KEYS_MODE && !g_InvKeysObjects) {
        g_InvChosen = -1;
        return GF_NOP;
    }
    Phase_Set(PHASE_INVENTORY, (void *)(intptr_t)inv_mode);
    return Phase_Run();
}
