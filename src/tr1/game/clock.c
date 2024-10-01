#include "game/clock.h"

#include "config.h"
#include "game/console/common.h"
#include "game/game_string.h"
#include "global/const.h"

#include <libtrx/utils.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

static double M_GetElapsedUnit(CLOCK_TIMER *const timer, const double unit);
static bool M_CheckElapsedUnit(
    CLOCK_TIMER *const timer, const double how_often, const double unit,
    bool bypass_turbo_cheat);

static double M_GetElapsedUnit(CLOCK_TIMER *const timer, const double unit)
{
    assert(timer != NULL);
    const double delta = Clock_GetHighPrecisionCounter() - timer->prev_counter;
    const double multiplier = Clock_GetSpeedMultiplier() / 1000.0;
    const double frames = delta * multiplier * unit;
    Clock_ResetTimer(timer);
    return frames;
}

static bool M_CheckElapsedUnit(
    CLOCK_TIMER *const timer, const double how_often, const double unit,
    bool bypass_turbo_cheat)
{
    assert(timer != NULL);
    const double delta = Clock_GetHighPrecisionCounter() - timer->prev_counter;
    const double multiplier =
        (bypass_turbo_cheat ? 1.0 : Clock_GetSpeedMultiplier()) / 1000.0;
    const double frames = delta * multiplier * unit;
    if (g_Config.rendering.fps != timer->prev_fps) {
        Clock_ResetTimer(timer);
        return false;
    }
    if (frames >= how_often) {
        Clock_ResetTimer(timer);
        return true;
    }
    return false;
}

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
    Config_Write();
    Console_Log(GS(OSD_SPEED_SET), value);
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

int32_t Clock_GetLogicalFrame(void)
{
    return Clock_GetHighPrecisionCounter() * LOGIC_FPS / 1000.0;
}

int32_t Clock_GetDrawFrame(void)
{
    return Clock_GetHighPrecisionCounter() * g_Config.rendering.fps / 1000.0;
}

void Clock_ResetTimer(CLOCK_TIMER *const timer)
{
    assert(timer != NULL);
    timer->prev_counter = Clock_GetHighPrecisionCounter();
    timer->prev_fps = g_Config.rendering.fps;
}

double Clock_GetElapsedLogicalFrames(CLOCK_TIMER *const timer)
{
    return M_GetElapsedUnit(timer, LOGIC_FPS);
}

double Clock_GetElapsedDrawFrames(CLOCK_TIMER *const timer)
{
    return M_GetElapsedUnit(timer, g_Config.rendering.fps);
}

double Clock_GetElapsedMilliseconds(CLOCK_TIMER *const timer)
{
    return M_GetElapsedUnit(timer, 1000.0);
}

bool Clock_CheckElapsedLogicalFrames(
    CLOCK_TIMER *const timer, const int32_t how_often)
{
    return M_CheckElapsedUnit(timer, how_often, LOGIC_FPS, false);
}

bool Clock_CheckElapsedDrawFrames(
    CLOCK_TIMER *const timer, const int32_t how_often)
{
    return M_CheckElapsedUnit(timer, how_often, g_Config.rendering.fps, false);
}

bool Clock_CheckElapsedMilliseconds(
    CLOCK_TIMER *const timer, const int32_t how_often)
{
    return M_CheckElapsedUnit(timer, how_often, 1000, false);
}

bool Clock_CheckElapsedRawMilliseconds(
    CLOCK_TIMER *const timer, const int32_t how_often)
{
    return M_CheckElapsedUnit(timer, how_often, 1000, true);
}
