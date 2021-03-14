#ifndef T1M_GAME_PEOPLE_H
#define T1M_GAME_PEOPLE_H

#include "game/const.h"
#include "game/types.h"
#include "util.h"

#include <stdint.h>

#define PEOPLE_POSE_CHANCE 0x60 // = 96
#define PEOPLE_SHOOT_RANGE SQUARE(WALL_L * 7) // = 51380224
#define PEOPLE_SHOT_DAMAGE 50
#define PEOPLE_WALK_TURN (PHD_DEGREE * 3) // = 546
#define PEOPLE_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define PEOPLE_WALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define PEOPLE_MISS_CHANCE 0x2000

int32_t Targetable(ITEM_INFO *item, AI_INFO *info);
int16_t GunShot(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num);
int16_t GunHit(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num);
int16_t GunMiss(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num);
int32_t ShotLara(
    ITEM_INFO *item, int32_t distance, BITE_INFO *gun, int16_t extra_rotation);

#endif
