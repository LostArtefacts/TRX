#pragma once

#include <trx/core/math/types.h>
#include <trx/game/items/types.h>
#include <trx/game/types.h>

#include <stdint.h>

#define FX_RIPPLE_ACTIVE 0x01u
#define FX_RIPPLE_SLOW 0x02u
#define FX_RIPPLE_DARK 0x10u
#define FX_RIPPLE_BLOOD 0x20u
#define FX_RIPPLE_JITTER 0x40u

typedef struct {
    XYZ_32 pos;
    int32_t prev_size;
    int32_t prev_life;
    int32_t prev_init;
    uint8_t flags;
    uint8_t life;
    uint8_t size;
    uint8_t init;
} FX_WATER_RIPPLE;

typedef struct {
    XYZ_16 pos;
    XYZ_16 prev_pos;
    XYZ_32 vel;
    // Gives the speed that friction decays the point toward.
    XZ_32 min_vel;
    uint8_t friction;
    uint8_t gravity;
} FX_WATER_SPLASH_VERT;

typedef struct {
    XYZ_32 pos;
    int32_t prev_life;
    uint8_t flags;
    uint8_t life;
    uint8_t pad[2];
    FX_WATER_SPLASH_VERT v[48];
} FX_WATER_SPLASH;

typedef struct {
    XYZ_32 pos;
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
} FX_WATER_SPLASH_SETUP;

FX_WATER_RIPPLE *FX_Water_SetupRipple(XYZ_32 pos, int32_t size, uint32_t flags);
void FX_Water_SetupSplash(const FX_WATER_SPLASH_SETUP *setup);
void FX_Water_Splash(const ITEM *item);
void FX_Water_WadeSplash(const ITEM *item, int32_t depth);

void FX_Water_TriggerUnderwaterBlood(XYZ_32 pos, int32_t size);
void FX_Water_TriggerUnderwaterBloodD(XYZ_32 pos, int32_t size);
