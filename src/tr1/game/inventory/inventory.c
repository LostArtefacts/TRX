#include "game/inventory/inventory_vars.h"
#include "game/phase/phase.h"
#include "game/phase/phase_inventory.h"
#include "global/types.h"

#include <libtrx/memory.h>

bool Inv_Display(const INV_MODE inv_mode)
{
    if (inv_mode == INV_KEYS_MODE && !g_InvKeysObjects) {
        return false;
    }
    PHASE_INVENTORY_ARGS *const args =
        Memory_Alloc(sizeof(PHASE_INVENTORY_ARGS));
    args->mode = inv_mode;
    Phase_Set(PHASE_INVENTORY, args);
    return true;
}
