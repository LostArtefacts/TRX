#include <trx/game/clock/common.h>

#include <emscripten.h>

void Clock_Delay(const int32_t ms)
{
    emscripten_sleep(ms);
}
