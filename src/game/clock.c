#include "game/clock.h"

#include "specific/s_clock.h"

#include <stdio.h>
#include <windows.h>

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

void Clock_GetDateTime(char *date_time)
{
    SYSTEMTIME lt = S_Clock_GetLocalTime();
    // sprintf(
    //     date_time, "%04d%02d%02d_%02d%02d%02d_%03f", Clock_GetYear(),
    //     Clock_GetMonth(), Clock_GetDay(), Clock_GetHours(),
    //     Clock_GetMinutes(), Clock_GetSeconds());
    sprintf(
        date_time, "%04d%02d%02d_%02d%02d%02d_%03d", lt.wYear, lt.wMonth,
        lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
}
