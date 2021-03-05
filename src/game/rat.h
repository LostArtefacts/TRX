#ifndef T1M_GAME_RAT_H
#define T1M_GAME_RAT_H

#include <stdint.h>

// clang-format off
#define VoleControl             ((void          (*)(int16_t item_num))0x00434210)
// clang-format on

void RatControl(int16_t item_num);

void T1MInjectGameRat();

#endif
