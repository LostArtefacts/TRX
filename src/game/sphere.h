#ifndef T1M_GAME_SPHERE_H
#define T1M_GAME_SPHERE_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define GetSpheres          ((int32_t       (*)(ITEM_INFO* item, SPHERE* ptr, int32_t world_space))0x00439260)
#define GetJointAbsPosition ((void          (*)(ITEM_INFO* item, PHD_VECTOR* vec, int32_t joint))0x00439550)
// clang-format on

#endif
