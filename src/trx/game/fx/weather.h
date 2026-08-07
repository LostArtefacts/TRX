#pragma once

typedef enum {
    WEATHER_NONE = 0,
    WEATHER_RAIN,
    WEATHER_SNOW,
} WEATHER_TYPE;

// The heaviest weather the particle pool can carry, as a multiple of the count
// the original games show.
#define WEATHER_SEVERITY_MAX 4

WEATHER_TYPE FX_Weather_GetWeather(void);
void FX_Weather_SetWeather(WEATHER_TYPE weather_type);

float FX_Weather_GetSeverity(void);

// Clamps to [0, WEATHER_SEVERITY_MAX].
void FX_Weather_SetSeverity(float severity);
