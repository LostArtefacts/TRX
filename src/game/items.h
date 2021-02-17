#ifndef TR1MAIN_GAME_ITEMS_H
#define TR1MAIN_GAME_ITEMS_H

// clang-format off
#define InitialiseItemArray     ((void          __cdecl(*)(int item_count))0x00421B10)
#define InitialiseItem          ((void          __cdecl(*)(int16_t item_num))0x00421CC0)
// clang-format on

void __cdecl InitialiseFXArray();

void TR1MInjectGameItems();

#endif
