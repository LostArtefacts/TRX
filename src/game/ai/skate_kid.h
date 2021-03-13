#ifndef T1M_GAME_AI_SKATE_KID_H
#define T1M_GAME_AI_SKATE_KID_H

#include "game/types.h"
#include <stdint.h>

#define KID_STOP_SHOT_DAMAGE 50
#define KID_SKATE_SHOT_DAMAGE 40
#define KID_STOP_RANGE SQUARE(WALL_L * 4) // = 16777216
#define KID_DONT_STOP_RANGE SQUARE(WALL_L * 5 / 2) // = 6553600
#define KID_TOO_CLOSE SQUARE(WALL_L) // = 1048576
#define KID_SKATE_TURN (PHD_DEGREE * 4) // = 728
#define KID_PUSH_CHANCE 0x200
#define KID_SKATE_CHANCE 0x400
#define KID_DIE_ANIM 13

typedef enum {
    KID_STOP = 0,
    KID_SHOOT = 1,
    KID_SKATE = 2,
    KID_PUSH = 3,
    KID_SHOOT2 = 4,
    KID_DEATH = 5,
} KID_ANIM;

extern BITE_INFO KidGun1;
extern BITE_INFO KidGun2;

void SetupSkateKid(OBJECT_INFO *obj);
void InitialiseSkateKid(int16_t item_num);
void SkateKidControl(int16_t item_num);
void DrawSkateKid(ITEM_INFO *item);

#endif
