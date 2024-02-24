#include "specific/s_clock.h"

#include "global/const.h"

#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>

static Uint64 m_Counter = 0;
static Uint64 m_Frequency = 0;

bool S_Clock_Init(void)
{
    m_Frequency = SDL_GetPerformanceFrequency();
    m_Counter = SDL_GetPerformanceCounter();
    return true;
}

int32_t S_Clock_GetMS(void)
{
    return SDL_GetTicks();
}

int32_t S_Clock_SyncTicks(void)
{
    const Uint64 last_counter = m_Counter;
    const double fps = TICKS_PER_SECOND;

    while (true) {
        m_Counter = SDL_GetPerformanceCounter();

        const Uint64 target_counter =
            last_counter + TICKS_PER_FRAME * (m_Frequency / fps);
        const double elapsed_sec =
            (double)(m_Counter - last_counter) / m_Frequency;
        const double delay_sec = m_Counter <= target_counter
            ? (double)(target_counter - m_Counter) / m_Frequency
            : 0.0;
        int32_t delay_ms = delay_sec * 1000;
        if (delay_ms > 0) {
            SDL_Delay(delay_ms);
        }

        const double elapsed_ticks = elapsed_sec * fps;
        if (elapsed_ticks >= (double)TICKS_PER_FRAME) {
            return elapsed_ticks;
        }
    }
}
