#ifndef TR1MAIN_GAME_DRAW_H
#define TR1MAIN_GAME_DRAW_H

// clang-format off
#define DrawEffect              ((void          __cdecl*(*)(int16_t fx_num))0x00417400)
// clang-format on

void __cdecl PrintRooms(int16_t room_number);

void TR1MInjectDraw();

#endif
