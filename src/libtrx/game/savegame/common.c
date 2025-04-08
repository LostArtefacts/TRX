#include "game/savegame.h"
#include "log.h"

static int32_t m_BoundSlot = -1;

void Savegame_BindSlot(const int32_t slot_num)
{
    m_BoundSlot = slot_num;
    LOG_DEBUG("Binding save slot %d", slot_num);
}

void Savegame_UnbindSlot(void)
{
    LOG_DEBUG("Resetting the save slot");
    m_BoundSlot = -1;
}

int32_t Savegame_GetBoundSlot(void)
{
    return m_BoundSlot;
}
