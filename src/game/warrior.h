#ifndef T1M_GAME_WARRIOR_H
#define T1M_GAME_WARRIOR_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define InitialisePod           ((void      (*)(int16_t item_num))0x0043CC70)
#define PodControl              ((void      (*)(int16_t item_num))0x0043CD70)
#define InitialiseStatue        ((void      (*)(int16_t item_num))0x0043CE90)
#define StatueControl           ((void      (*)(int16_t item_num))0x0043CF80)
// clang-format on

void CentaurControl(int16_t item_num);
void InitialiseWarrior2(int16_t item_num);
void FlyerControl(int16_t item_num);

void ControlMissile(int16_t fx_num);
void ShootAtLara(FX_INFO* fx);
int16_t ShardGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num);
int16_t RocketGun(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);

void InitialiseMummy(int16_t item_num);
void MummyControl(int16_t item_num);

int32_t ExplodingDeath(int16_t item_num, int32_t mesh_bits, int16_t damage);
void ControlBodyPart(int16_t fx_num);

void T1MInjectGameWarrior();

#endif
