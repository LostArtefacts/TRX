#ifndef T1M_GAME_AI_WOLF_H
#define T1M_GAME_AI_WOLF_H

#include "game/types.h"

#include <stdint.h>

#define WOLF_SLEEP_FRAME 96
#define WOLF_BITE_DAMAGE 100
#define WOLF_POUNCE_DAMAGE 50
#define WOLF_DIE_ANIM 20
#define WOLF_WALK_TURN (2 * PHD_DEGREE) // = 364
#define WOLF_RUN_TURN (5 * PHD_DEGREE) // = 910
#define WOLF_STALK_TURN (2 * PHD_DEGREE) // = 364
#define WOLF_ATTACK_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define WOLF_STALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define WOLF_BITE_RANGE SQUARE(345) // = 119025
#define WOLF_WAKE_CHANCE 32
#define WOLF_SLEEP_CHANCE 32
#define WOLF_HOWL_CHANCE 384
#define WOLF_TOUCH 0x774F
#define WOLF_HITPOINTS 6
#define WOLF_RADIUS (WALL_L / 3) // = 341
#define WOLF_SMARTNESS 0x2000

typedef enum {
    WOLF_EMPTY = 0,
    WOLF_STOP = 1,
    WOLF_WALK = 2,
    WOLF_RUN = 3,
    WOLF_JUMP = 4,
    WOLF_STALK = 5,
    WOLF_ATTACK = 6,
    WOLF_HOWL = 7,
    WOLF_SLEEP = 8,
    WOLF_CROUCH = 9,
    WOLF_FASTTURN = 10,
    WOLF_DEATH = 11,
    WOLF_BITE = 12,
} WOLF_ANIM;

extern BITE_INFO WolfJawBite;

void SetupWolf(OBJECT_INFO *obj);
void InitialiseWolf(int16_t item_num);
void WolfControl(int16_t item_num);

#endif
