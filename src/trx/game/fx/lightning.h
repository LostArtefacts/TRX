#pragma once

#include <trx/core/colors.h>
#include <trx/core/math/types.h>

#include <stdint.h>

typedef enum {
    FX_LIGHTNING_F_NONE = 0,
    // The bolt bends in a smooth curve. Without this flag, it runs in a
    // jagged line from end to end.
    FX_LIGHTNING_F_CURVED = 1 << 0,
    // The far end of the bolt drifts while the bolt lasts.
    FX_LIGHTNING_F_MOVING_END = 1 << 1,
} FX_LIGHTNING_FLAGS;

typedef struct {
    XYZ_32 src;
    XYZ_32 dst;
    // How far the bolt strays from a straight line. Must be above zero.
    uint8_t variation;
    RGB_888 color;
    // The bolt loses two life each tick, and fades out over its last 16.
    uint8_t life;
    // How wide the glow around the bolt is.
    uint8_t size;
    // How many curve sections make up the bolt.
    uint8_t segments;
    FX_LIGHTNING_FLAGS flags;
} FX_LIGHTNING_SETUP;

// Starts a TR4 lightning bolt: a bright line with a colored glow around it.
// Does nothing while all the bolts are in use.
void FX_Lightning_Create(const FX_LIGHTNING_SETUP *setup);

// Lights a short flash at the position, as TR4 does where a bolt strikes.
// Does nothing where Lara is more than 16 sectors away along X or Z.
void FX_Lightning_CreateGlow(XYZ_32 pos, RGB_888 color, uint8_t size);
