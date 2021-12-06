#ifndef T1M_GAME_AI_MUTANT_H
#define T1M_GAME_AI_MUTANT_H

#include "global/types.h"

#include <stdint.h>

#define FLYER_CHARGE_DAMAGE 100
#define FLYER_LUNGE_DAMAGE 150
#define FLYER_PUNCH_DAMAGE 200
#define FLYER_PART_DAMAGE 100
#define FLYER_WALK_TURN (PHD_DEGREE * 2) // = 364
#define FLYER_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define FLYER_POSE_CHANCE 80
#define FLYER_UNPOSE_CHANCE 256
#define FLYER_WALK_RANGE SQUARE(WALL_L * 9 / 2) // = 21233664
#define FLYER_ATTACK1_RANGE SQUARE(600) // = 360000
#define FLYER_ATTACK2_RANGE SQUARE(WALL_L * 5 / 2) // = 6553600
#define FLYER_ATTACK3_RANGE SQUARE(300) // = 90000
#define FLYER_ATTACK_RANGE SQUARE(WALL_L * 15 / 4) // = 14745600
#define FLYER_TOUCH 0x678
#define FLYER_BULLET1 1
#define FLYER_BULLET2 2
#define FLYER_FLYMODE 4
#define FLYER_TWIST 8
#define FLYER_HITPOINTS 50
#define FLYER_RADIUS (WALL_L / 3) // = 341
#define FLYER_SMARTNESS 0x7FFF

#define WARRIOR2_SMARTNESS 0x2000

enum FLYER_ANIM {
    FLYER_EMPTY = 0,
    FLYER_STOP = 1,
    FLYER_WALK = 2,
    FLYER_RUN = 3,
    FLYER_ATTACK1 = 4,
    FLYER_DEATH = 5,
    FLYER_POSE = 6,
    FLYER_ATTACK2 = 7,
    FLYER_ATTACK3 = 8,
    FLYER_AIM1 = 9,
    FLYER_AIM2 = 10,
    FLYER_SHOOT = 11,
    FLYER_MUMMY = 12,
    FLYER_FLY = 13,
};

extern BITE_INFO g_WarriorBite;
extern BITE_INFO g_WarriorRocket;
extern BITE_INFO g_WarriorShard;

void SetupWarrior(OBJECT_INFO *obj);
void SetupWarrior2(OBJECT_INFO *obj);
void SetupWarrior3(OBJECT_INFO *obj);

void FlyerControl(int16_t item_num);

void InitialiseWarrior2(int16_t item_num);

#endif
