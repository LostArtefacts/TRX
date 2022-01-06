#include "game/clock.h"

#include "specific/s_clock.h"

#include <stdio.h>

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

int32_t Clock_GetYear()
{
    return S_Clock_GetYear();
}

int32_t Clock_GetMonth()
{
    return S_Clock_GetMonth();
}

int32_t Clock_GetDay()
{
    return S_Clock_GetDay();
}

int32_t Clock_GetHours()
{
    return S_Clock_GetHours();
}

int32_t Clock_GetMinutes()
{
    return S_Clock_GetMinutes();
}

int32_t Clock_GetSeconds()
{
    return S_Clock_GetSeconds();
}

char *Clock_GetDateTime(char *date_time)
{
#include "log.h"
    sprintf(
        date_time, "%04d%02d%02d_%02d%02d%02d", Clock_GetYear(),
        Clock_GetMonth(), Clock_GetDay(), Clock_GetHours(), Clock_GetMinutes(),
        Clock_GetSeconds());
    return date_time;
}