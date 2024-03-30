#include "config.h"
#include "game/clock.h"

#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <stdbool.h>
#include <stdint.h>

static Uint64 m_LastCounter = 0;
static Uint64 m_Counter = 0;
static Uint64 m_Frequency = 0;

void Clock_Init(void)
{
    m_Frequency = SDL_GetPerformanceFrequency();
    m_Counter = SDL_GetPerformanceCounter();
}

int32_t Clock_SyncTicks(void)
{
    m_LastCounter = m_Counter;
    const double fps = g_Config.rendering.fps;

    const double frequency = (double)m_Frequency / Clock_GetSpeedMultiplier();
    const Uint64 target_counter = m_LastCounter + (frequency / fps);

    while (true) {
        m_Counter = SDL_GetPerformanceCounter();

        const double elapsed_sec =
            (double)(m_Counter - m_LastCounter) / frequency;
        const double delay_sec = m_Counter <= target_counter
            ? (double)(target_counter - m_Counter) / frequency
            : 0.0;
        int32_t delay_ms = delay_sec * 1000;
        if (delay_ms > 0) {
            SDL_Delay(delay_ms);
        }

        const double elapsed_ticks = elapsed_sec * fps;
        if (elapsed_ticks >= 1) {
            return elapsed_ticks;
        }
    }
}

int32_t Clock_GetMS(void)
{
    return SDL_GetTicks();
}
