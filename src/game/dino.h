#ifndef T1M_GAME_DINO_H
#define T1M_GAME_DINO_H

#include <stdint.h>

// clang-format off
#define LaraDinoDeath           ((void         (*)(ITEM_INFO* item))0x004163A0)
// clang-format on

void RaptorControl(int16_t item_num);
void DinoControl(int16_t item_num);

void T1MInjectGameDino();

#endif
