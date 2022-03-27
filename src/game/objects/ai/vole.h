#pragma once

#include "global/types.h"

#include <stdint.h>

#define VOLE_DIE_ANIM 2
#define VOLE_SWIM_TURN (PHD_DEGREE * 3) // = 546
#define VOLE_ATTACK_RANGE SQUARE(300) // = 90000

typedef enum {
    VOLE_EMPTY = 0,
    VOLE_SWIM = 1,
    VOLE_ATTACK = 2,
    VOLE_DEATH = 3,
} VOLE_ANIM;

void Vole_Setup(OBJECT_INFO *obj);
void Vole_Control(int16_t item_num);
