#pragma once

#include <stdint.h>

typedef enum {
    WEATHER_NONE = 0,
    WEATHER_RAIN,
    WEATHER_SNOW,
} WEATHER_TYPE;

void WeatherFX_Init(void);
void WeatherFX_Update(void);
void WeatherFX_Draw(void);
void WeatherFX_SetWeather(WEATHER_TYPE weather_type);
