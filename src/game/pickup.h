#ifndef T1M_GAME_PICKUP_H
#define T1M_GAME_PICKUP_H

#include <stdint.h>

// clang-format off
#define SwitchTrigger           ((int32_t      (*)(int16_t item_num, int16_t timer))0x00433E20)
#define KeyTrigger              ((int32_t      (*)(int16_t item_num))0x00433EA0)
#define PickupTrigger           ((int32_t      (*)(int16_t item_num))0x00433EF0)
// clang-format on

#endif
