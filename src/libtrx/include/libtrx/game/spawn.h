#pragma once

#include "./items/types.h"
#include "./types.h"

void Spawn_Splash(const ITEM *item);
void Spawn_Ricochet(const GAME_VECTOR *pos);

void Spawn_Bubble(const XYZ_32 *pos, int16_t room_num);

int16_t Spawn_Blood(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);
int16_t Spawn_GunShot(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);
int16_t Spawn_GunHit(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);
int16_t Spawn_GunMiss(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);
