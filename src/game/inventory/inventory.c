#include "game/inventory/inventory_vars.h"
#include "game/phase/phase.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stdint.h>

bool Inv_Display(const INV_MODE inv_mode)
{
    if (inv_mode == INV_KEYS_MODE && !g_InvKeysObjects) {
        g_InvChosen = -1;
        return false;
    }
    Phase_Set(PHASE_INVENTORY, (void *)(intptr_t)inv_mode);
    return true;
}
