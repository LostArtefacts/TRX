#include "game/random.h"

#include "global/funcs.h"
#include "global/vars.h"

#include <time.h>

static int32_t m_RandControl = 0xD371F947;
static int32_t m_RandDraw = 0xD371F947;

void __cdecl Random_SeedControl(const int32_t seed)
{
    m_RandControl = seed;
}

void __cdecl Random_SeedDraw(const int32_t seed)
{
    m_RandDraw = seed;
}

int32_t __cdecl Random_GetControl(void)
{
    m_RandControl = 0x41C64E6D * m_RandControl + 12345;
    return (m_RandControl >> 10) & 0x7FFF;
}

int32_t __cdecl Random_GetDraw(void)
{
    m_RandDraw = 0x41C64E6D * m_RandDraw + 12345;
    return (m_RandDraw >> 10) & 0x7FFF;
}

void __cdecl Random_Seed(void)
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);
    Random_SeedControl(tptr->tm_sec + 57 * tptr->tm_min + 3543 * tptr->tm_hour);
    Random_SeedDraw(tptr->tm_sec + 43 * tptr->tm_min + 3477 * tptr->tm_hour);
}
