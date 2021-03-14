#ifndef T1M_GAME_AI_SKATE_KID_H
#define T1M_GAME_AI_SKATE_KID_H

#include "game/types.h"

#include <stdint.h>

#define SKATE_KID_STOP_SHOT_DAMAGE 50
#define SKATE_KID_SKATE_SHOT_DAMAGE 40
#define SKATE_KID_STOP_RANGE SQUARE(WALL_L * 4) // = 16777216
#define SKATE_KID_DONT_STOP_RANGE SQUARE(WALL_L * 5 / 2) // = 6553600
#define SKATE_KID_TOO_CLOSE SQUARE(WALL_L) // = 1048576
#define SKATE_KID_SKATE_TURN (PHD_DEGREE * 4) // = 728
#define SKATE_KID_PUSH_CHANCE 0x200
#define SKATE_KID_SKATE_CHANCE 0x400
#define SKATE_KID_DIE_ANIM 13
#define SKATE_KID_HITPOINTS 125
#define SKATE_KID_RADIUS (WALL_L / 5) // = 204
#define SKATE_KID_SMARTNESS 0x7FFF

typedef enum {
    SKATE_KID_STOP = 0,
    SKATE_KID_SHOOT = 1,
    SKATE_KID_SKATE = 2,
    SKATE_KID_PUSH = 3,
    SKATE_KID_SHOOT2 = 4,
    SKATE_KID_DEATH = 5,
} SKATE_KID_ANIM;

extern BITE_INFO KidGun1;
extern BITE_INFO KidGun2;

void SetupSkateKid(OBJECT_INFO *obj);
void InitialiseSkateKid(int16_t item_num);
void SkateKidControl(int16_t item_num);
void DrawSkateKid(ITEM_INFO *item);

#endif
