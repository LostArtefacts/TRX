#pragma once

#include <stdint.h>

// clang-format off
#pragma pack(push, 1)

typedef struct {
    int32_t boat_turn;
    int32_t left_fallspeed;
    int32_t right_fallspeed;
    int16_t tilt_angle;
    int16_t extra_rotation;
    int32_t water;
    int32_t pitch;
} BOAT_INFO;

typedef struct {
    int16_t track_mesh;
    int32_t skidoo_turn;
    int32_t left_fallspeed;
    int32_t right_fallspeed;
    int16_t momentum_angle;
    int16_t extra_rotation;
    int32_t pitch;
} SKIDOO_INFO;

#pragma pack(pop)

// clang-format on
