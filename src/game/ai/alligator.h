#ifndef T1M_GAME_AI_ALLIGATOR_H
#define T1M_GAME_AI_ALLIGATOR_H

#include "game/types.h"

#include <stdint.h>

#define ALLIGATOR_BITE_DAMAGE 100
#define ALLIGATOR_DIE_ANIM 4
#define ALLIGATOR_FLOAT_SPEED (WALL_L / 32) // = 32
#define ALLIGATOR_TURN (3 * PHD_DEGREE) // = 546
#define ALLIGATOR_HITPOINTS 20
#define ALLIGATOR_RADIUS (WALL_L / 3) // = 341
#define ALLIGATOR_SMARTNESS 0x400

typedef enum {
    ALLIGATOR_EMPTY = 0,
    ALLIGATOR_SWIM = 1,
    ALLIGATOR_ATTACK = 2,
    ALLIGATOR_DEATH = 3,
} ALLIGATOR_ANIM;

void SetupAlligator(OBJECT_INFO *obj);
void AlligatorControl(int16_t item_num);

#endif
