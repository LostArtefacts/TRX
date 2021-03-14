#ifndef T1M_GAME_AI_BALDY_H
#define T1M_GAME_AI_BALDY_H

#include "game/types.h"

#include <stdint.h>

#define BALDY_SHOT_DAMAGE 150
#define BALDY_WALK_TURN (PHD_DEGREE * 3) // = 546
#define BALDY_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define BALDY_WALK_RANGE SQUARE(WALL_L * 4) // = 16777216
#define BALDY_DIE_ANIM 14
#define BALDY_HITPOINTS 200
#define BALDY_RADIUS (WALL_L / 10) // = 102
#define BALDY_SMARTNESS 0x7FFF

typedef enum {
    BALDY_EMPTY = 0,
    BALDY_STOP = 1,
    BALDY_WALK = 2,
    BALDY_RUN = 3,
    BALDY_AIM = 4,
    BALDY_DEATH = 5,
    BALDY_SHOOT = 6,
} BALDY_ANIM;

extern BITE_INFO BaldyGun;

void SetupBaldy(OBJECT_INFO *obj);
void InitialiseBaldy(int16_t item_num);
void BaldyControl(int16_t item_num);

#endif
