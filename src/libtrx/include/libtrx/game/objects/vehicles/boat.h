#pragma once

#include <stdint.h>

typedef struct {
    int32_t boat_turn;
    int32_t left_fallspeed;
    int32_t right_fallspeed;
    int16_t tilt_angle;
    int16_t extra_rotation;
    int32_t water;
    int32_t pitch;
} BOAT_INFO;
