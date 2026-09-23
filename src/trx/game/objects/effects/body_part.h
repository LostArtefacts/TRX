#pragma once

#include <trx/core/math.h>
#include <trx/game/effects/types.h>
#include <trx/game/items/enum.h>

#include <stdint.h>

typedef struct {
    XYZ_32 pos;
    int16_t room_num;
    int16_t mesh_idx;
    int16_t shade;
    GIB_FLAGS gib_flags;
    int16_t speed;
    int16_t fall_speed;
    int16_t flame_variant;
    int16_t damage;
} BODY_PART_ARGS;

// Creates one flying body part from the specified arguments.
EFFECT *BodyPart_Create(const BODY_PART_ARGS *args);
