#ifndef T1M_GAME_AI_COWBOY_H
#define T1M_GAME_AI_COWBOY_H

#include "game/types.h"
#include <stdint.h>

#define COWBOY_SHOT_DAMAGE 70
#define COWBOY_WALK_TURN (PHD_DEGREE * 3) // = 546
#define COWBOY_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define COWBOY_WALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define COWBOY_DIE_ANIM 7
#define COWBOY_HITPOINTS 150
#define COWBOY_RADIUS (WALL_L / 10) // = 102
#define COWBOY_SMARTNESS 0x7FFF

typedef enum {
    COWBOY_EMPTY = 0,
    COWBOY_STOP = 1,
    COWBOY_WALK = 2,
    COWBOY_RUN = 3,
    COWBOY_AIM = 4,
    COWBOY_DEATH = 5,
    COWBOY_SHOOT = 6,
} COWBOY_ANIM;

extern BITE_INFO CowboyGun1;
extern BITE_INFO CowboyGun2;

void SetupCowboy(OBJECT_INFO *obj);
void CowboyControl(int16_t item_num);

#endif
