#include <fakes/level.h>

#include <harness/fake_calls.h>

#include <trx/game/level/common.h>

static bool m_WorldLoaded = true;

static void M_Reset(void)
{
    m_WorldLoaded = true;
}

FAKE_ON_RESET(M_Reset)

void FakeLevel_SetWorldLoaded(const bool loaded)
{
    m_WorldLoaded = loaded;
}

bool Level_IsWorldLoaded(void)
{
    return m_WorldLoaded;
}
