#ifndef T1M_GAME_AI_RAT_H
#define T1M_GAME_AI_RAT_H

#include "global/types.h"

#include <stdint.h>

#define RAT_BITE_DAMAGE 20
#define RAT_CHARGE_DAMAGE 20
#define RAT_TOUCH 0x300018F
#define RAT_DIE_ANIM 8
#define RAT_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define RAT_BITE_RANGE SQUARE(341) // = 116281
#define RAT_CHARGE_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define RAT_POSE_CHANCE 256
#define RAT_HITPOINTS 5
#define RAT_RADIUS (WALL_L / 5) // = 204
#define RAT_SMARTNESS 0x2000

typedef enum {
    RAT_EMPTY = 0,
    RAT_STOP = 1,
    RAT_ATTACK2 = 2,
    RAT_RUN = 3,
    RAT_ATTACK1 = 4,
    RAT_DEATH = 5,
    RAT_POSE = 6,
} RAT_ANIM;

extern BITE_INFO g_RatBite;

void SetupRat(OBJECT_INFO *obj);
void RatControl(int16_t item_num);

#endif
