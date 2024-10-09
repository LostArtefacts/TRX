#pragma once

#include <stdint.h>

int32_t __cdecl Effect_ExplodingDeath(
    int16_t item_num, int32_t mesh_bits, int16_t damage);

int16_t __cdecl Effect_MissileFlame(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);

void __cdecl Effect_CreateBartoliLight(int16_t item_num);
