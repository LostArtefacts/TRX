#ifndef T1M_GAME_AI_APE_H
#define T1M_GAME_AI_APE_H

#include "game/types.h"
#include <stdint.h>

#define APE_ATTACK_DAMAGE 200
#define APE_TOUCH 0xFF00
#define APE_DIE_ANIM 7
#define APE_RUN_TURN (PHD_DEGREE * 5) // = 910
#define APE_DISPLAY_ANGLE (PHD_DEGREE * 45) // = 8190
#define APE_ATTACK_RANGE SQUARE(430) // = 184900
#define APE_PANIC_RANGE SQUARE(WALL_L * 2) // = 4194304
#define APE_JUMP_CHANCE 160
#define APE_WARN1_CHANCE (APE_JUMP_CHANCE + 160) // = 320
#define APE_WARN2_CHANCE (APE_WARN1_CHANCE + 160) // = 480
#define APE_RUN_LEFT_CHANCE (APE_WARN2_CHANCE + 272) // = 752
#define APE_ATTACK_FLAG 1
#define APE_VAULT_ANIM 19
#define APE_TURN_L_FLAG 2
#define APE_TURN_R_FLAG 4
#define APE_SHIFT 75

typedef enum {
    APE_EMPTY = 0,
    APE_STOP = 1,
    APE_WALK = 2,
    APE_RUN = 3,
    APE_ATTACK1 = 4,
    APE_DEATH = 5,
    APE_WARNING = 6,
    APE_WARNING2 = 7,
    APE_RUN_LEFT = 8,
    APE_RUN_RIGHT = 9,
    APE_JUMP = 10,
    APE_VAULT = 11,
} APE_ANIM;

extern BITE_INFO ApeBite;

void SetupApe(OBJECT_INFO *obj);
void ApeVault(int16_t item_num, int16_t angle);
void ApeControl(int16_t item_num);

#endif
