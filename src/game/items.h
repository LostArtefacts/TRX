#ifndef TOMB1MAIN_GAME_ITEMS_H
#define TOMB1MAIN_GAME_ITEMS_H

// clang-format off
#define InitialiseItemArray     ((void          __cdecl(*)(int item_count))0x00421B10)
#define InitialiseItem          ((void          __cdecl(*)(int16_t item_num))0x00421CC0)
#define GlobalItemReplace       ((int32_t       __cdecl(*)(int32_t in_objnum, int32_t out_objnum))0x004221D0)
// clang-format on

void __cdecl InitialiseFXArray();

void Tomb1MInjectGameItems();

#endif
