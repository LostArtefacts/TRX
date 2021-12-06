#include "game/clock.h"

#include "specific/s_clock.h"

bool Clock_Init()
{
    return S_Clock_Init();
}

int32_t Clock_GetMS()
{
    return S_Clock_GetMS();
}

int32_t Clock_Sync()
{
    return S_Clock_Sync();
}

int32_t Clock_SyncTicks(int32_t target)
{
    return S_Clock_SyncTicks(target);
}
