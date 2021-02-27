#ifndef T1M_GAME_WARRIOR_H
#define T1M_GAME_WARRIOR_H

#include <stdint.h>

// clang-format off
#define CentaurControl          ((void          (*)(int16_t item_num))0x0043B850)
#define InitialiseWarrior2      ((void          (*)(int16_t item_num))0x0043BB30)
#define FlyerControl            ((void          (*)(int16_t item_num))0x0043BB60)
#define InitialiseMummy         ((void          (*)(int16_t item_num))0x0043C650)
#define MummyControl            ((void          (*)(int16_t item_num))0x0043C690)
#define InitialisePod           ((void          (*)(int16_t item_num))0x0043CC70)
#define PodControl              ((void          (*)(int16_t item_num))0x0043CD70)
#define InitialiseStatue        ((void          (*)(int16_t item_num))0x0043CE90)
#define StatueControl           ((void          (*)(int16_t item_num))0x0043CF80)
// clang-format on

#endif
