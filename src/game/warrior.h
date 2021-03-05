#ifndef T1M_GAME_WARRIOR_H
#define T1M_GAME_WARRIOR_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define CentaurControl          ((void      (*)(int16_t item_num))0x0043B850)
#define InitialiseWarrior2      ((void      (*)(int16_t item_num))0x0043BB30)
#define FlyerControl            ((void      (*)(int16_t item_num))0x0043BB60)
#define ControlMissile          ((void      (*)(int16_t item_num))0x0043C1C0)
#define ShardGun                ((int16_t   (*)(int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE yrot, int16_t room_num))0x0043C430)
#define RocketGun               ((int16_t   (*)(int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE yrot, int16_t room_num))0x0043C540)
#define InitialiseMummy         ((void      (*)(int16_t item_num))0x0043C650)
#define MummyControl            ((void      (*)(int16_t item_num))0x0043C690)
#define ExplodingDeath          ((int32_t   (*)(int16_t item_num, int32_t mesh_bits, int16_t damage))0x0043C730)
#define ControlBodyPart         ((void      (*)(int16_t item_num))0x0043CAD0)
#define InitialisePod           ((void      (*)(int16_t item_num))0x0043CC70)
#define PodControl              ((void      (*)(int16_t item_num))0x0043CD70)
#define InitialiseStatue        ((void      (*)(int16_t item_num))0x0043CE90)
#define StatueControl           ((void      (*)(int16_t item_num))0x0043CF80)
// clang-format on

#endif
