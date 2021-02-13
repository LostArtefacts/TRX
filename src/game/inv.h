#ifndef TR1M_GAME_INV_H
#define TR1M_GAME_INV_H

// clang-format off
#define Inv_RemoveItem          ((void          __cdecl(*)(int16_t item_num))0x004212A0)
#define Inv_RequestItem         ((int32_t       __cdecl(*)(int16_t item_num))0x00421200)
#define Display_Inventory       ((int32_t       __cdecl(*)(int32_t inventory_mode))0x0041E760)
// clang-format on

#endif
