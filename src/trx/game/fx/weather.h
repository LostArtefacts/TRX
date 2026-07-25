#pragma once

#include <trx/core/math/types.h>

#include <stdint.h>

typedef enum {
    WEATHER_NONE = 0,
    WEATHER_RAIN,
    WEATHER_SNOW,
} WEATHER_TYPE;

typedef struct {
    XYZ_32 pos;
    XYZ_32 prev_pos;
    int32_t prev_yv;
    int8_t xv;
    uint8_t yv;
    int8_t zv;
    uint8_t life;
} FX_RAINDROP;

typedef struct {
    XYZ_32 pos;
    XYZ_32 prev_pos;
    bool stopped;
    int32_t prev_yv;
    int32_t prev_life;
    int8_t xv;
    uint8_t yv;
    int8_t zv;
    uint8_t life;
} FX_SNOWFLAKE;

void FX_Weather_Reset(void);
void FX_Weather_Control(void);
void FX_Weather_Draw(void);
WEATHER_TYPE FX_Weather_GetWeather(void);
void FX_Weather_SetWeather(WEATHER_TYPE weather_type);

// The particle accessors return nullptr past the end of their pool. A zero
// pos.x marks a free slot.
FX_RAINDROP *FX_Weather_GetRaindrop(int32_t idx);
FX_SNOWFLAKE *FX_Weather_GetSnowflake(int32_t idx);
