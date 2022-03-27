#pragma once

#include "global/types.h"

#include <stdint.h>

#define RAPTOR_ATTACK_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define RAPTOR_BITE_DAMAGE 100
#define RAPTOR_CHARGE_DAMAGE 100
#define RAPTOR_CLOSE_RANGE SQUARE(680) // = 462400
#define RAPTOR_DIE_ANIM 9
#define RAPTOR_LUNGE_DAMAGE 100
#define RAPTOR_LUNGE_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define RAPTOR_ROAR_CHANCE 256
#define RAPTOR_RUN_TURN (4 * PHD_DEGREE) // = 728
#define RAPTOR_TOUCH 0xFF7C00
#define RAPTOR_WALK_TURN (1 * PHD_DEGREE) // = 182
#define RAPTOR_HITPOINTS 20
#define RAPTOR_RADIUS (WALL_L / 3) // = 341
#define RAPTOR_SMARTNESS 0x4000

typedef enum {
    RAPTOR_EMPTY = 0,
    RAPTOR_STOP = 1,
    RAPTOR_WALK = 2,
    RAPTOR_RUN = 3,
    RAPTOR_ATTACK1 = 4,
    RAPTOR_DEATH = 5,
    RAPTOR_WARNING = 6,
    RAPTOR_ATTACK2 = 7,
    RAPTOR_ATTACK3 = 8,
} RAPTOR_ANIM;

extern BITE_INFO g_RaptorBite;

void Raptor_Setup(OBJECT_INFO *obj);
void Raptor_Control(int16_t item_num);
