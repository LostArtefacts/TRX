#pragma once

#include "global/types.h"

#include <libtrx/game/spawn.h>

void Spawn_Splash(ITEM *item);

void Spawn_Bubble(const XYZ_32 *pos, int16_t room_num);

int16_t Spawn_ShardGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);

int16_t Spawn_RocketGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);

int16_t Spawn_GunShot(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);

int16_t Spawn_GunShotHit(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);

int16_t Spawn_GunShotMiss(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);

void Spawn_Ricochet(GAME_VECTOR *pos);

void Spawn_Twinkle(GAME_VECTOR *pos);
