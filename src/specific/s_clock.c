#include "specific/s_clock.h"

#include "global/vars.h"

#include <windows.h>

static LONGLONG Ticks = 0;
static double Frequency = 0;

int32_t ClockSyncTicks(int32_t target)
{
    double elapsed = 0.0;
    uint64_t last_ticks = Ticks;
    do {
        ClockUpdateTicks();
        elapsed = (double)(Ticks - last_ticks) / Frequency;
    } while (elapsed < (double)target);
    return elapsed;
}

void ClockUpdateTicks()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    Ticks = counter.QuadPart;
}

int8_t ClockInit()
{
    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency)) {
        return 0;
    }

    Frequency = (double)frequency.QuadPart / (double)TICKS_PER_SECOND;
    ClockUpdateTicks();
    return 1;
}

int32_t ClockSync()
{
    LONGLONG last_ticks = Ticks;
    ClockUpdateTicks();
    return ((double)(Ticks - last_ticks) / Frequency);
}

int32_t ClockGetMS()
{
    return GetTickCount();
}
