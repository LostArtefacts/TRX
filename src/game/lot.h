#ifndef TOMB1MAIN_GAME_LOT_H
#define TOMB1MAIN_GAME_LOT_H

// clang-format off
#define InitialiseLOT           ((void          __cdecl(*)())0x0042A780)
#define EnableBaddieAI          ((int32_t       __cdecl(*)(int16_t item_num, int32_t))0x0042A3A0)
// clang-format on

void __cdecl InitialiseLOTArray();

void Tomb1MInjectGameLOT();

#endif
