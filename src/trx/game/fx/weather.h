#pragma once

typedef enum {
    WEATHER_NONE = 0,
    WEATHER_RAIN,
    WEATHER_SNOW,
} WEATHER_TYPE;

WEATHER_TYPE FX_Weather_GetWeather(void);
void FX_Weather_SetWeather(WEATHER_TYPE weather_type);
