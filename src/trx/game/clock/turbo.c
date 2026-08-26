#include <trx/game/clock/turbo.h>

#include <trx/config.h>
#include <trx/core/utils.h>
#include <trx/game/clock/common.h>
#include <trx/game/clock/timer.h>
#include <trx/game/console/common.h>
#include <trx/game/game_strings/entries.h>

#include <math.h>

static bool m_IsHeld = false;

void Clock_CycleTurboSpeed(const bool forward)
{
    int32_t new_speed = Clock_GetTurboSpeed() + (forward ? 1 : -1);
    if (new_speed < CLOCK_TURBO_SPEED_MIN
        || new_speed > CLOCK_TURBO_SPEED_MAX) {
        new_speed = 0;
    }
    Clock_SetTurboSpeed(new_speed);
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
    CONFIG_SET(g_Config.gameplay.turbo_speed, value);
    Config_Update();
    Console_Info(GS("general/osd/speed_set"), value);
    Clock_SetSimSpeed(Clock_GetSpeedMultiplier());
}

void Clock_HoldTurboSpeed(int32_t value)
{
    CLAMP(value, CLOCK_TURBO_SPEED_MIN, CLOCK_TURBO_SPEED_MAX);
    if (m_IsHeld) {
        if (value == g_Config.gameplay.turbo_speed) {
            return;
        }
        CONFIG_POP_HOLD(g_Config.gameplay.turbo_speed);
    }
    CONFIG_PUSH_HOLD(g_Config.gameplay.turbo_speed, value, CONFIG_HOLD_INPUT);
    m_IsHeld = true;
    Config_Update();
}

void Clock_ReleaseTurboSpeed(void)
{
    if (!m_IsHeld) {
        return;
    }
    CONFIG_POP_HOLD(g_Config.gameplay.turbo_speed);
    m_IsHeld = false;
    Config_Update();
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
