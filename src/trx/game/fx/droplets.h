#pragma once

#include <trx/game/types.h>

// Releases a TR4 water drop that falls as a short streak until it reaches
// the floor or water. Nothing is released while water droplets are
// disabled.
void FX_Droplet_Create(
    XYZ_32 pos, int16_t room_num, RGB_888 color, int16_t y_vel, uint8_t gravity,
    uint8_t life);
