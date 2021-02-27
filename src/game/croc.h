#ifndef T1M_GAME_CROC_H
#define T1M_GAME_CROC_H

#include <stdint.h>

// clang-format off
#define CrocControl             ((void         (*)(int16_t item_num))0x00415850)
// clang-format on

void AlligatorControl(int16_t item_num);

void T1MInjectGameCroc();

#endif
