// IWYU pragma: no_include <bits/types/struct_tm.h>
#include "game/clock.h"

#include "game/console.h"
#include "game/game_string.h"
#include "game/interpolation.h"
#include "game/phase/phase.h"
#include "global/vars.h"
#include "log.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

#define CLOCK_TURBO_SPEED_MIN -2
#define CLOCK_TURBO_SPEED_MAX 2

static double m_Progress = 0.0;

int32_t Clock_GetTurboSpeed(void)
{
    return g_Config.rendering.turbo_speed;
}

void Clock_CycleTurboSpeed(const bool forward)
{
    Clock_SetTurboSpeed(g_Config.rendering.turbo_speed + (forward ? 1 : -1));
}

void Clock_SetTurboSpeed(int32_t value)
{
    CLAMP(value, CLOCK_TURBO_SPEED_MIN, CLOCK_TURBO_SPEED_MAX);
    g_Config.rendering.turbo_speed = value;
    Console_Log(GS(OSD_SPEED_SET), value);
    Config_Write();
}

double Clock_GetSpeedMultiplier(void)
{
    if (g_Config.rendering.turbo_speed > 0) {
        return 1.0 + g_Config.rendering.turbo_speed;
    } else if (g_Config.rendering.turbo_speed < 0) {
        return pow(2.0, g_Config.rendering.turbo_speed);
    } else {
        return 1.0;
    }
}

void Clock_SetTickProgress(double progress)
{
    m_Progress = progress;
}

double Clock_GetTickProgress(void)
{
    if (!Interpolation_IsEnabled()) {
        return 1.0;
    }
    return m_Progress;
}

int32_t Clock_GetLogicalFrame(void)
{
    return Clock_GetMS() * LOGIC_FPS / 1000;
}

int32_t Clock_GetDrawFrame(void)
{
    return Clock_GetMS() * g_Config.rendering.fps / 1000;
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

int32_t Clock_GetFrameAdvance(void)
{
    return g_Config.rendering.fps == 30 ? 2 : 1;
}

double Clock_GetFrameAdvanceAdjusted(void)
{
    double frames = Clock_GetFrameAdvance();
    if (Phase_Get() != PHASE_INVENTORY || g_InvMode != INV_TITLE_MODE) {
        frames /= 2.0;
    }
    return frames;
}

bool Clock_IsAtLogicalFrame(const int32_t how_often)
{
    const int32_t multiplier = g_Config.rendering.fps / LOGIC_FPS;
    return Clock_GetDrawFrame() % (how_often * multiplier) == 0;
}
