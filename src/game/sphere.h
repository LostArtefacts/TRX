#ifndef T1M_GAME_SPHERE_H
#define T1M_GAME_SPHERE_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define BaddieBiteEffect    ((void          (*)(ITEM_INFO *item, BITE_INFO *bite))0x004396F0)
// clang-format on

int32_t TestCollision(ITEM_INFO* item, ITEM_INFO* lara_item);
int32_t GetSpheres(ITEM_INFO* item, SPHERE* slist, int32_t world_space);
void GetJointAbsPosition(ITEM_INFO* item, PHD_VECTOR* vec, int32_t joint);

void T1MInjectGameSphere();

#endif
