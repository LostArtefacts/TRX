#include "specific/s_clock.h"

#include "config.h"
#include "global/vars.h"

#include <SDL2/SDL.h>

static Uint64 m_Ticks = 0;
static double m_Frequency = 0.0;

static void m_UpdateTicks(void);

static void m_UpdateTicks(void)
{
    m_Ticks = SDL_GetPerformanceCounter();
}

bool S_Clock_Init(void)
{
    m_Frequency =
        (double)SDL_GetPerformanceFrequency() / (double)TICKS_PER_SECOND;
    m_UpdateTicks();
    return true;
}

int32_t S_Clock_GetMS(void)
{
    return SDL_GetTicks();
}

int32_t S_Clock_Sync(void)
{
    Uint64 last_ticks = m_Ticks;
    m_UpdateTicks();
    return ((double)(m_Ticks - last_ticks) / m_Frequency);
}

int32_t S_Clock_SyncTicks(int32_t target)
{
    double elapsed = 0.0;
    Uint64 last_ticks = m_Ticks;
    do {
        m_UpdateTicks();
        elapsed = (double)(m_Ticks - last_ticks) / m_Frequency;
    } while (elapsed < (double)target);
    return elapsed;
}
