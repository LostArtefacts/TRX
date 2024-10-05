#include "game/phase/phase.h"
#include "game/phase/phase_inventory.h"
#include "global/types.h"

#include <libtrx/memory.h>

GAMEFLOW_COMMAND Game_MainMenu(void)
{
    PHASE_INVENTORY_ARGS *const args =
        Memory_Alloc(sizeof(PHASE_INVENTORY_ARGS));
    args->mode = INV_TITLE_MODE;
    Phase_Set(PHASE_INVENTORY, args);
    return Phase_Run();
}
