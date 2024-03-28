// IWYU pragma: no_include <bits/types/struct_tm.h>
#include "game/clock.h"

#include "game/phase/phase.h"
#include "global/vars.h"
#include "log.h"

#include <stdio.h>
#include <time.h>

#define TURBO_SPEED_COUNT 5
#define TURBO_SPEED_OFFSET -2
#define TURBO_SPEED_DEFAULT_IDX 2

static double m_Progress = 0.0;
static int16_t m_TurboSpeedIdx = TURBO_SPEED_DEFAULT_IDX;

static double m_TurboSpeeds[TURBO_SPEED_COUNT] = {
    0.25, 0.5, 1.0, 2.0, 3.0,
};

void Clock_CycleTurboSpeed(bool forward)
{
    if (forward) {
        m_TurboSpeedIdx++;
    } else {
        m_TurboSpeedIdx--;
        m_TurboSpeedIdx %= TURBO_SPEED_COUNT;
        m_TurboSpeedIdx += TURBO_SPEED_COUNT;
    }
    m_TurboSpeedIdx %= TURBO_SPEED_COUNT;
}

void Clock_SetTurboSpeed(const int32_t idx)
{
    m_TurboSpeedIdx = idx - TURBO_SPEED_OFFSET;
    CLAMP(m_TurboSpeedIdx, 0, TURBO_SPEED_COUNT - 1);
    LOG_INFO("Setting speed to %d (%.2f)", idx, Clock_GetSpeedMultiplier());
}

int32_t Clock_GetTurboSpeed(void)
{
    return m_TurboSpeedIdx + TURBO_SPEED_OFFSET;
}

double Clock_GetSpeedMultiplier(void)
{
    return m_TurboSpeeds[m_TurboSpeedIdx];
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
