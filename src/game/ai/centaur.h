#ifndef T1M_GAME_AI_CENTAUR_H
#define T1M_GAME_AI_CENTAUR_H

#include "global/types.h"

#include <stdint.h>

#define CENTAUR_PART_DAMAGE 100
#define CENTAUR_REAR_DAMAGE 200
#define CENTAUR_TOUCH 0x30199
#define CENTAUR_DIE_ANIM 8
#define CENTAUR_TURN (PHD_DEGREE * 4) // = 728
#define CENTAUR_REAR_CHANCE 96
#define CENTAUR_REAR_RANGE SQUARE(WALL_L * 3 / 2) // = 2359296
#define CENTAUR_HITPOINTS 120
#define CENTAUR_RADIUS (WALL_L / 3) // = 341
#define CENTAUR_SMARTNESS 0x7FFF

typedef enum {
    CENTAUR_EMPTY = 0,
    CENTAUR_STOP = 1,
    CENTAUR_SHOOT = 2,
    CENTAUR_RUN = 3,
    CENTAUR_AIM = 4,
    CENTAUR_DEATH = 5,
    CENTAUR_WARNING = 6,
} CENTAUR_ANIM;

extern BITE_INFO g_CentaurRocket;
extern BITE_INFO g_CentaurRear;

void SetupCentaur(OBJECT_INFO *obj);
void CentaurControl(int16_t item_num);

#endif
