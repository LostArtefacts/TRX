#ifndef T1M_GAME_PICKUP_H
#define T1M_GAME_PICKUP_H

#include <stdint.h>

// clang-format off
#define SwitchTrigger           ((int32_t       (*)(int16_t item_num, int16_t timer))0x00433E20)
#define PickupTrigger           ((int32_t       (*)(int16_t item_num))0x00433EF0)
// clang-format on

int32_t KeyTrigger(int16_t item_num);

void T1MInjectGamePickup();

#endif
