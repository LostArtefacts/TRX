#ifndef T1M_GAME_AI_LARSON_H
#define T1M_GAME_AI_LARSON_H

#include "game/types.h"
#include <stdint.h>

#define LARSON_POSE_CHANCE 0x60 // = 96
#define LARSON_SHOT_DAMAGE 50
#define LARSON_WALK_TURN (PHD_DEGREE * 3) // = 546
#define LARSON_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define LARSON_WALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define LARSON_DIE_ANIM 15
#define LARSON_HITPOINTS 50
#define LARSON_RADIUS (WALL_L / 10) // = 102
#define LARSON_SMARTNESS 0x7FFF

typedef enum {
    LARSON_EMPTY = 0,
    LARSON_STOP = 1,
    LARSON_WALK = 2,
    LARSON_RUN = 3,
    LARSON_AIM = 4,
    LARSON_DEATH = 5,
    LARSON_POSE = 6,
    LARSON_SHOOT = 7,
} LARSON_ANIM;

extern BITE_INFO LarsonGun;

void SetupLarson(OBJECT_INFO *obj);
void LarsonControl(int16_t item_num);

#endif
