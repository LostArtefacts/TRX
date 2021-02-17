#ifndef TR1MAIN_GAME_DRAW_H
#define TR1MAIN_GAME_DRAW_H

#include <stdint.h>

// clang-format off
#define DrawEffect              ((void          __cdecl(*)(int16_t fx_num))0x00417400)
#define DrawAnimatingItem       ((void          __cdecl(*)(ITEM_INFO *item))0x00417550)
#define GetBoundsAccurate       ((int16_t*      __cdecl(*)(ITEM_INFO* item))0x00419DD0)
// clang-format on

void __cdecl PrintRooms(int16_t room_number);

void TR1MInjectGameDraw();

#endif
