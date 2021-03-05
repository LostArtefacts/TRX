#ifndef T1M_GAME_PEOPLE_H
#define T1M_GAME_PEOPLE_H

#include "game/types.h"
#include <stdint.h>

// clang-format off
#define PierreControl           ((void      (*)(int16_t item_num))0x00431550)
#define ApeControl              ((void      (*)(int16_t item_num))0x00431D40)
#define InitialiseSkateKid      ((void      (*)(int16_t item_num))0x004320B0)
#define SkateKidControl         ((void      (*)(int16_t item_num))0x004320E0)
#define DrawSkateKid            ((void      (*)(ITEM_INFO *item))0x00432550)
#define CowboyControl           ((void      (*)(int16_t item_num))0x004325A0)
#define InitialiseBaldy         ((void      (*)(int16_t item_num))0x00432B60)
#define BaldyControl            ((void      (*)(int16_t item_num))0x00432B90)
// clang-format on

int32_t Targetable(ITEM_INFO* item, AI_INFO* info);
void ControlGunShot(int16_t fx_num);
int16_t GunShot(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num);
int16_t GunHit(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num);
int16_t GunMiss(
    int32_t x, int32_t y, int32_t z, int16_t speed, PHD_ANGLE y_rot,
    int16_t room_num);

int32_t ShotLara(
    ITEM_INFO* item, int32_t distance, BITE_INFO* gun, int16_t extra_rotation);
void PeopleControl(int16_t item_num);

void T1MInjectGamePeople();

#endif
