#pragma once

#include <stdint.h>

typedef struct {
    int32_t velocity;
    int16_t front_rot;
    int16_t rear_rot;
    int32_t revs;
    int32_t engine_revs;
    int16_t track_mesh;
    int32_t skidoo_turn;
    int32_t left_fall_speed;
    int32_t right_fall_speed;
    int16_t momentum_angle;
    int16_t extra_rotation;
    int32_t pitch;
    uint8_t flags;
} QUAD_BIKE_INFO;

bool QuadBike_Control(void);
