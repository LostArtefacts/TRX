#include "game/savegame.h"

#include "global/vars.h"

#include <libtrx/config.h>

int32_t Savegame_GetSlotCount(void)
{
    return g_Config.gameplay.maximum_save_slots;
}

void Savegame_HighlightNewestSlot(void)
{
    g_GameInfo.select_save_slot = Savegame_GetMostRecentlyCreatedSlot();
}
