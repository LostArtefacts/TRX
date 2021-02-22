#ifndef TOMB1MAIN_GAME_WARRIOR_H
#define TOMB1MAIN_GAME_WARRIOR_H

#include <stdint.h>

// clang-format off
#define CentaurControl          ((void          __cdecl(*)(int16_t item_num))0x0043B850)
#define InitialiseWarrior2      ((void          __cdecl(*)(int16_t item_num))0x0043BB30)
#define FlyerControl            ((void          __cdecl(*)(int16_t item_num))0x0043BB60)
#define InitialiseMummy         ((void          __cdecl(*)(int16_t item_num))0x0043C650)
#define MummyControl            ((void          __cdecl(*)(int16_t item_num))0x0043C690)
#define InitialisePod           ((void          __cdecl(*)(int16_t item_num))0x0043CC70)
#define PodControl              ((void          __cdecl(*)(int16_t item_num))0x0043CD70)
#define InitialiseStatue        ((void          __cdecl(*)(int16_t item_num))0x0043CE90)
#define StatueControl           ((void          __cdecl(*)(int16_t item_num))0x0043CF80)
// clang-format on

#endif
