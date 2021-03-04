#ifndef T1M_GAME_NATLA_H
#define T1M_GAME_NATLA_H

#include <stdint.h>

// clang-format off
#define ControlNatlaGun         ((void          (*)(int16_t item_num))0x0042C910)
// clang-format on

void AbortionControl(int16_t item_num);
void NatlaControl(int16_t item_num);

void T1MInjectGameNatla();

#endif
