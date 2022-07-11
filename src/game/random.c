#include "game/random.h"

#include "log.h"

static int32_t m_RandControl = 0xD371F947;
static int32_t m_RandDraw = 0xD371F947;

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
    m_RandDraw = 0x41C64E6D * m_RandDraw + 0x3039;
    return (m_RandDraw >> 10) & 0x7FFF;
}
