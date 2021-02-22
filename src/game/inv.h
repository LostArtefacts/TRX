#ifndef TOMB1MAIN_GAME_INV_H
#define TOMB1MAIN_GAME_INV_H

#include <stdint.h>

// clang-format off
#define Inv_AddItem             ((int32_t       __cdecl(*)(int16_t item_num))0x004209C0)
#define Inv_RemoveAllItems      ((void          __cdecl(*)())0x00421280)
#define Inv_RemoveItem          ((void          __cdecl(*)(int16_t item_num))0x004212A0)
#define Inv_RequestItem         ((int32_t       __cdecl(*)(int16_t item_num))0x00421200)
#define Display_Inventory       ((int32_t       __cdecl(*)(int32_t inventory_mode))0x0041E760)
// clang-format on

#endif
