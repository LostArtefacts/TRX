#ifndef T1M_GAME_SPHERE_H
#define T1M_GAME_SPHERE_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define GetSpheres          ((int32_t       (*)(ITEM_INFO* item, SPHERE* ptr, int32_t world_space))0x00439260)
#define GetJointAbsPosition ((void          (*)(ITEM_INFO* item, PHD_VECTOR* vec, int32_t joint))0x00439550)
#define BaddieBiteEffect    ((void          (*)(ITEM_INFO *item, BITE_INFO *bite))0x004396F0)
// clang-format on

int32_t TestCollision(ITEM_INFO* item, ITEM_INFO* lara_item);

void T1MInjectGameSphere();

#endif
