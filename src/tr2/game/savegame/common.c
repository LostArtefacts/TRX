#include <stdint.h>

// TODO: make configurable (legacy MAX_REQUESTER_ITEMS)
#define MAX_SAVE_SLOTS 24

int32_t Savegame_GetSlotCount(void)
{
    return MAX_SAVE_SLOTS;
}

void Savegame_HighlightNewestSlot(void)
{
}
