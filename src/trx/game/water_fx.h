#pragma once

#include <trx/game/items/types.h>
#include <trx/game/types.h>

#include <stdint.h>

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
    uint8_t flags;
    uint8_t life;
    uint8_t size;
    uint8_t init;
} WATER_FX_RIPPLE;

typedef struct {
    int16_t wx;
    int16_t wy;
    int16_t wz;
    int16_t xv;
    int32_t yv;
    int16_t zv;
    int16_t oxv;
    int16_t ozv;
    uint8_t friction;
    uint8_t gravity;
} WATER_FX_SPLASH_VERT;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
    uint8_t flags;
    uint8_t life;
    uint8_t pad[2];
    WATER_FX_SPLASH_VERT v[48];
} WATER_FX_SPLASH;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
    int16_t inner_xz_off;
    int16_t inner_xz_size;
    int16_t inner_y_size;
    int16_t inner_xz_vel;
    int16_t inner_y_vel;
    int16_t inner_gravity;
    int16_t inner_friction;
    int16_t middle_xz_off;
    int16_t middle_xz_size;
    int16_t middle_y_size;
    int16_t middle_xz_vel;
    int16_t middle_y_vel;
    int16_t middle_gravity;
    int16_t middle_friction;
    int16_t outer_xz_off;
    int16_t outer_xz_size;
    int16_t outer_xz_vel;
    int16_t outer_friction;
} WATER_FX_SPLASH_SETUP;

void WaterFX_Init(void);
void WaterFX_Update(void);
void WaterFX_Draw(void);

WATER_FX_RIPPLE *WaterFX_SetupRipple(
    int32_t x, int32_t y, int32_t z, int32_t size, bool is_still);
void WaterFX_SetupSplash(const WATER_FX_SPLASH_SETUP *setup);
void WaterFX_Splash(const ITEM *item);
void WaterFX_WadeSplash(const ITEM *item, int32_t water_height, int32_t depth);
