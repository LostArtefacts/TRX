#ifndef T1M_GAME_NATLA_H
#define T1M_GAME_NATLA_H

#include <stdint.h>

// clang-format off
#define AbortionControl         ((void          __cdecl(*)(int16_t item_num))0x0042BE60)
#define NatlaControl            ((void          __cdecl(*)(int16_t item_num))0x0042C330)
// clang-format on

#endif
