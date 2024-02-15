// IWYU pragma: no_include <bits/types/struct_tm.h>
#include "game/clock.h"

#include "specific/s_clock.h"

#include <stdio.h>
#include <time.h>

#define MAX_TURBO_SPEED_MUL 3

static int16_t m_TurboSpeedMul = 1;

void Clock_CycleTurboSpeed(void)
{
    if (m_TurboSpeedMul >= MAX_TURBO_SPEED_MUL) {
        m_TurboSpeedMul = 1;
    } else {
        m_TurboSpeedMul++;
    }
}

bool Clock_Init(void)
{
    return S_Clock_Init();
}

int32_t Clock_GetMS(void)
{
    return S_Clock_GetMS();
}

int32_t Clock_SyncTicks(void)
{
    return S_Clock_SyncTicks() * m_TurboSpeedMul;
}

void Clock_GetDateTime(char *date_time)
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);

    sprintf(
        date_time, "%04d%02d%02d_%02d%02d%02d", tptr->tm_year + 1900,
        tptr->tm_mon + 1, tptr->tm_mday, tptr->tm_hour, tptr->tm_min,
        tptr->tm_sec);
}
