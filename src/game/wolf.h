#ifndef T1M_GAME_WOLF_H
#define T1M_GAME_WOLF_H

#include <stdint.h>

// clang-format off
#define WolfControl             ((void          (*)(int16_t item_num))0x0043DF50)
#define LionControl             ((void          (*)(int16_t item_num))0x0043E390)
// clang-format on

void InitialiseWolf(int16_t item_num);

void T1MInjectGameWolf();

#endif
