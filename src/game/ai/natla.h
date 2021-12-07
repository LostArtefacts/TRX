#pragma once

#include "global/types.h"

#include <stdint.h>

#define NATLA_SHOT_DAMAGE 100
#define NATLA_NEAR_DEATH 200
#define NATLA_FLY_MODE 0x8000
#define NATLA_TIMER 0x7FFF
#define NATLA_FIRE_ARC (PHD_DEGREE * 30) // = 5460
#define NATLA_FLY_TURN (PHD_DEGREE * 5) // = 910
#define NATLA_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define NATLA_LAND_CHANCE 256
#define NATLA_DIE_TIME (FRAMES_PER_SECOND * 16) // = 480
#define NATLA_GUN_SPEED 400
#define NATLA_HITPOINTS 400
#define NATLA_RADIUS (WALL_L / 5) // = 204
#define NATLA_SMARTNESS 0x7FFF

typedef enum {
    NATLA_EMPTY = 0,
    NATLA_STOP = 1,
    NATLA_FLY = 2,
    NATLA_RUN = 3,
    NATLA_AIM = 4,
    NATLA_SEMIDEATH = 5,
    NATLA_SHOOT = 6,
    NATLA_FALL = 7,
    NATLA_STAND = 8,
    NATLA_DEATH = 9,
} NATLA_ANIM;

extern BITE_INFO g_NatlaGun;

void SetupNatla(OBJECT_INFO *obj);
void SetupNatlaGun(OBJECT_INFO *obj);
void NatlaControl(int16_t item_num);
void ControlNatlaGun(int16_t fx_num);
