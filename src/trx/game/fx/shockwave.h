#pragma once

#include <trx/core/colors.h>
#include <trx/core/math/types.h>

#include <stdint.h>

typedef enum {
    FX_SHOCKWAVE_F_NONE = 0,
    // The ring hurts Lara as it passes through her.
    FX_SHOCKWAVE_F_HURT = 1 << 0,
    // The ring hurts Lara half as much as FX_SHOCKWAVE_F_HURT does.
    FX_SHOCKWAVE_F_HURT_LESS = 1 << 1,
} FX_SHOCKWAVE_FLAGS;

typedef struct {
    XYZ_32 pos;
    int16_t inner_radius;
    int16_t outer_radius;
    // How far the outer edge moves on the first tick. The inner edge moves
    // half as far, and both slow down as the ring grows.
    int16_t speed;
    // How far the ring tilts about the X axis.
    int16_t x_rot;
    RGB_888 color;
    // How many ticks the ring lasts. It fades out over the last eight.
    uint8_t life;
    FX_SHOCKWAVE_FLAGS flags;
} FX_SHOCKWAVE_SETUP;

// Starts a TR4 shockwave: a flat ring that grows from a point and fades.
void FX_Shockwave_Create(const FX_SHOCKWAVE_SETUP *setup);
