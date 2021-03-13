#ifndef T1M_GAME_AI_CROC_H
#define T1M_GAME_AI_CROC_H

#include "game/types.h"
#include <stdint.h>

#define CROC_BITE_DAMAGE 100
#define CROC_BITE_RANGE SQUARE(435) // = 189225
#define CROC_DIE_ANIM 11
#define CROC_FASTTURN_ANGLE 0x4000
#define CROC_FASTTURN_RANGE SQUARE(WALL_L * 3) // = 9437184
#define CROC_FASTTURN_TURN (6 * PHD_DEGREE) // = 1092
#define CROC_TOUCH 0x3FC
#define CROC_TURN (3 * PHD_DEGREE) // = 546

typedef enum {
    CROC_EMPTY = 0,
    CROC_STOP = 1,
    CROC_RUN = 2,
    CROC_WALK = 3,
    CROC_FASTTURN = 4,
    CROC_ATTACK1 = 5,
    CROC_ATTACK2 = 6,
    CROC_DEATH = 7,
} CROC_ANIM;

extern BITE_INFO CrocBite;

void SetupCroc(OBJECT_INFO *obj);
void CrocControl(int16_t item_num);

#endif
