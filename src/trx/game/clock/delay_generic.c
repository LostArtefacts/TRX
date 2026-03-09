#include <trx/game/clock/common.h>

#include <SDL2/SDL_timer.h>

void Clock_Delay(const int32_t ms)
{
    if (ms > 0) {
        SDL_Delay((Uint32)ms);
    }
}
