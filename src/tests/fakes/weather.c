// The weather as the two variables FX_Weather keeps: what falls, and how
// heavily. The clamp on the severity is the engine's, so a test sees the range
// a script gets.

#include <harness/fake_calls.h>

#include <trx/core/utils.h>
#include <trx/game/fx/weather.h>

static WEATHER_TYPE m_Weather;
static float m_Severity;

static void M_Reset(void)
{
    m_Weather = WEATHER_NONE;
    m_Severity = 1.0f;
}

void FX_Weather_SetWeather(const WEATHER_TYPE weather_type)
{
    m_Weather = weather_type;
}

WEATHER_TYPE FX_Weather_GetWeather(void)
{
    return m_Weather;
}

void FX_Weather_SetSeverity(const float severity)
{
    m_Severity = severity;
    CLAMP(m_Severity, 0.0f, (float)WEATHER_SEVERITY_MAX);
}

float FX_Weather_GetSeverity(void)
{
    return m_Severity;
}

FAKE_ON_RESET(M_Reset)
