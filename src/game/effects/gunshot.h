#pragma once

#include "global/types.h"

#include <stdint.h>

void GunShot_Setup(OBJECT_INFO *obj);
void GunShot_Control(int16_t fx_num);

int16_t GunShot_Spawn(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num);
int16_t GunShot_SpawnHit(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num);
int16_t GunShot_SpawnMiss(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num);
