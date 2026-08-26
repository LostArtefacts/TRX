#include <fakes/level.h>

#include <trx/game/level/common.h>

static bool m_WorldLoaded = true;

void FakeLevel_SetWorldLoaded(const bool loaded)
{
    m_WorldLoaded = loaded;
}

bool Level_IsWorldLoaded(void)
{
    return m_WorldLoaded;
}
