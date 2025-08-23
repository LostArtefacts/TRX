#include "game/clock/turbo.h"

#include "config.h"
#include "game/clock/common.h"
#include "game/clock/timer.h"
#include "game/console/common.h"
#include "game/game_string.h"
#include "utils.h"

#include <math.h>

void Clock_CycleTurboSpeed(const bool forward)
{
    Clock_SetTurboSpeed(Clock_GetTurboSpeed() + (forward ? 1 : -1));
}

int32_t Clock_GetTurboSpeed(void)
{
    return g_Config.gameplay.turbo_speed;
}

void Clock_SetTurboSpeed(int32_t value)
{
    CLAMP(value, CLOCK_TURBO_SPEED_MIN, CLOCK_TURBO_SPEED_MAX);
    if (value == g_Config.gameplay.turbo_speed) {
        return;
    }
    g_Config.gameplay.turbo_speed = value;
    Config_Update();
    Console_Log(GS(OSD_SPEED_SET), value);
    Clock_SetSimSpeed(Clock_GetSpeedMultiplier());
}

double Clock_GetSpeedMultiplier(void)
{
    if (Clock_GetTurboSpeed() > 0) {
        return 1.0 + Clock_GetTurboSpeed();
    } else if (Clock_GetTurboSpeed() < 0) {
        return pow(2.0, Clock_GetTurboSpeed());
    } else {
        return 1.0;
    }
}
