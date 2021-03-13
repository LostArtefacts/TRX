#ifndef T1M_GAME_AI_LION_H
#define T1M_GAME_AI_LION_H

#include "game/types.h"
#include <stdint.h>

#define LION_BITE_DAMAGE 250
#define LION_POUNCE_DAMAGE 150
#define LION_TOUCH 0x380066
#define LION_WALK_TURN (2 * PHD_DEGREE) // = 364
#define LION_RUN_TURN (5 * PHD_DEGREE) // = 910
#define LION_ROAR_CHANCE 128
#define LION_POUNCE_RANGE SQUARE(WALL_L) // = 1048576
#define LION_DIE_ANIM 7

#define PUMA_DIE_ANIM 4

typedef enum {
    LION_EMPTY = 0,
    LION_STOP = 1,
    LION_WALK = 2,
    LION_RUN = 3,
    LION_ATTACK1 = 4,
    LION_DEATH = 5,
    LION_WARNING = 6,
    LION_ATTACK2 = 7,
} LION_ANIM;

extern BITE_INFO LionBite;

void SetupLion(OBJECT_INFO *obj);
void SetupLioness(OBJECT_INFO *obj);
void SetupPuma(OBJECT_INFO *obj);
void LionControl(int16_t item_num);

#endif
