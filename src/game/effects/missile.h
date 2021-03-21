#ifndef T1M_GAME_EFFECTS_MISSILE_H
#define T1M_GAME_EFFECTS_MISSILE_H

#include "global/types.h"

#include <stdint.h>

#define SHARD_DAMAGE 30
#define SHARD_SPEED 250
#define ROCKET_DAMAGE 100
#define ROCKET_RANGE SQUARE(WALL_L) // = 1048576
#define ROCKET_SPEED 220

void SetupMissile(OBJECT_INFO *obj);
void ControlMissile(int16_t fx_num);
void ShootAtLara(FX_INFO *fx);
int16_t ShardGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num);
int16_t RocketGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);

#endif
