#ifndef T1M_GAME_AI_VOLE_H
#define T1M_GAME_AI_VOLE_H

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

void SetupVole(OBJECT_INFO *obj);
void VoleControl(int16_t item_num);

#endif
