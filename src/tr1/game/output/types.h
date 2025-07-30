#pragma once

#include <libtrx/game/math/types.h>
#include <libtrx/game/output/types.h>

typedef enum {
    LIGHTING_MODE_OFF,
    LIGHTING_MODE_ONLY_SHADES,
    LIGHTING_MODE_FULL,
} LIGHTING_MODE;

typedef struct {
    XYZ_32 from;
    XYZ_32 to;
    int32_t thickness;
} LIGHTNING_SEGMENT;
