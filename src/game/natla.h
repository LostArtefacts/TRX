#ifndef T1M_GAME_NATLA_H
#define T1M_GAME_NATLA_H

#include <stdint.h>

void AbortionControl(int16_t item_num);
void NatlaControl(int16_t item_num);
void ControlNatlaGun(int16_t fx_num);

void T1MInjectGameNatla();

#endif
