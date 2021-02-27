#ifndef T1M_GAME_DINO_H
#define T1M_GAME_DINO_H

#include <stdint.h>

// clang-format off
#define DinoControl             ((void         (*)(int16_t item_num))0x004160F0)
// clang-format on

void RaptorControl(int16_t item_num);

void T1MInjectGameDino();

#endif
