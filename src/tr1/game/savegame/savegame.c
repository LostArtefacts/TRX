#include "game/savegame.h"

#include <libtrx/config.h>
#include <libtrx/game/option/passport.h>

int32_t Savegame_GetSlotCount(void)
{
    return g_Config.gameplay.maximum_save_slots;
}

void Savegame_HighlightNewestSlot(void)
{
    g_Passport.select_slot = Savegame_GetMostRecentlyCreatedSlot();
}
