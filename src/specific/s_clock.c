#include "specific/s_clock.h"

#include "global/const.h"

#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>

static Uint64 m_Ticks = 0;
static double m_Frequency = 0.0;
static Uint64 m_TicksPerMilliSecond = 1;

static void S_Clock_UpdateTicks(void);

static void S_Clock_UpdateTicks(void)
{
    m_Ticks = SDL_GetPerformanceCounter();
}

bool S_Clock_Init(void)
{
    m_TicksPerMilliSecond = SDL_GetPerformanceFrequency() / 1000;

    m_Frequency =
        (double)SDL_GetPerformanceFrequency() / (double)TICKS_PER_SECOND;
    S_Clock_UpdateTicks();
    return true;
}

int32_t S_Clock_GetMS(void)
{
    return SDL_GetTicks();
}

int32_t S_Clock_Sync(void)
{
    Uint64 last_ticks = m_Ticks;
    S_Clock_UpdateTicks();
    return ((double)(m_Ticks - last_ticks) / m_Frequency);
}

int32_t S_Clock_SyncTicks(int32_t target)
{
    double elapsed = 0.0;
    Uint64 last_ticks = m_Ticks;

    do {
        Uint64 target_ticks = last_ticks + target * m_Frequency;
        S_Clock_UpdateTicks();
        elapsed = (double)(m_Ticks - last_ticks) / m_Frequency;

        int delay = (target_ticks - m_Ticks) / m_TicksPerMilliSecond;
        if (delay > 0) {
            SDL_Delay(delay);
        }
    } while (elapsed < (double)target);

    return elapsed;
}
