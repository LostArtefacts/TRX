#include "game/random.h"

#include "log.h"

#include <time.h>

static int32_t m_RandControl = 0xD371F947;
static int32_t m_RandDraw = 0xD371F947;
static bool m_IsDrawFrozen = false;

void Random_Seed(void)
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);
    Random_SeedControl(tptr->tm_sec + 57 * tptr->tm_min + 3543 * tptr->tm_hour);
    Random_SeedDraw(tptr->tm_sec + 43 * tptr->tm_min + 3477 * tptr->tm_hour);
}

void Random_SeedControl(int32_t seed)
{
    LOG_DEBUG("%d", seed);
    m_RandControl = seed;
}

int32_t Random_GetControl(void)
{
    m_RandControl = 0x41C64E6D * m_RandControl + 0x3039;
    return (m_RandControl >> 10) & 0x7FFF;
}

void Random_SeedDraw(int32_t seed)
{
    LOG_DEBUG("%d", seed);
    m_RandDraw = seed;
}

int32_t Random_GetDraw(void)
{
    // Allow draw RNG to advance only during initial game setup (for such things
    // as caustic initialisation) and normal game play. RNG should remain static
    // when the game output is paused e.g. inventory, pause screen etc.
    if (!m_IsDrawFrozen) {
        m_RandDraw = 0x41C64E6D * m_RandDraw + 0x3039;
    }
    return (m_RandDraw >> 10) & 0x7FFF;
}

int32_t Random_GetControlSeed(void)
{
    return m_RandControl;
}

int32_t Random_GetDrawSeed(void)
{
    return m_RandDraw;
}

void Random_FreezeDraw(bool is_frozen)
{
    m_IsDrawFrozen = is_frozen;
}
