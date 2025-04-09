#include "game/savegame.h"
#include "log.h"

static SAVEGAME_VERSION m_InitialVersion = VERSION_LEGACY;
static int32_t m_BoundSlot = -1;

SAVEGAME_VERSION Savegame_GetInitialVersion(void)
{
    return m_InitialVersion;
}

void Savegame_SetInitialVersion(const SAVEGAME_VERSION version)
{
    m_InitialVersion = version;
}

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
