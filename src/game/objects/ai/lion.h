#pragma once

#include "global/types.h"

#include <stdint.h>

#define LION_BITE_DAMAGE 250
#define LION_POUNCE_DAMAGE 150
#define LION_TOUCH 0x380066
#define LION_WALK_TURN (2 * PHD_DEGREE) // = 364
#define LION_RUN_TURN (5 * PHD_DEGREE) // = 910
#define LION_ROAR_CHANCE 128
#define LION_POUNCE_RANGE SQUARE(WALL_L) // = 1048576
#define LION_DIE_ANIM 7
#define LION_HITPOINTS 30
#define LION_RADIUS (WALL_L / 3) // = 341
#define LION_SMARTNESS 0x7FFF

#define LIONESS_HITPOINTS 25
#define LIONESS_RADIUS (WALL_L / 3) // = 341
#define LIONESS_SMARTNESS 0x2000

#define PUMA_DIE_ANIM 4
#define PUMA_HITPOINTS 45
#define PUMA_RADIUS (WALL_L / 3) // = 341
#define PUMA_SMARTNESS 0x2000

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

extern BITE_INFO g_LionBite;

void Lion_SetupLion(OBJECT_INFO *obj);
void Lion_SetupLioness(OBJECT_INFO *obj);
void Lion_SetupPuma(OBJECT_INFO *obj);
void Lion_Control(int16_t item_num);
