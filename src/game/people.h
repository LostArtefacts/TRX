#ifndef T1M_GAME_PEOPLE_H
#define T1M_GAME_PEOPLE_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define PeopleControl           ((void          __cdecl(*)(int16_t item_num))0x00431090)
#define PierreControl           ((void          __cdecl(*)(int16_t item_num))0x00431550)
#define ApeControl              ((void          __cdecl(*)(int16_t item_num))0x00431D40)
#define InitialiseSkateKid      ((void          __cdecl(*)(int16_t item_num))0x004320B0)
#define SkateKidControl         ((void          __cdecl(*)(int16_t item_num))0x004320E0)
#define DrawSkateKid            ((void          __cdecl(*)(ITEM_INFO *item))0x00432550)
#define CowboyControl           ((void          __cdecl(*)(int16_t item_num))0x004325A0)
#define InitialiseBaldy         ((void          __cdecl(*)(int16_t item_num))0x00432B60)
#define BaldyControl            ((void          __cdecl(*)(int16_t item_num))0x00432B90)
// clang-format on

#endif
