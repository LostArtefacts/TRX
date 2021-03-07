#ifndef T1M_GAME_PEOPLE_H
#define T1M_GAME_PEOPLE_H

#include "game/types.h"
#include <stdint.h>

int32_t Targetable(ITEM_INFO *item, AI_INFO *info);
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
    ITEM_INFO *item, int32_t distance, BITE_INFO *gun, int16_t extra_rotation);
void PeopleControl(int16_t item_num);
void PierreControl(int16_t item_num);

void ApeVault(int16_t item_num, int16_t angle);
void ApeControl(int16_t item_num);

void InitialiseSkateKid(int16_t item_num);
void SkateKidControl(int16_t item_num);
void DrawSkateKid(ITEM_INFO *item);

void CowboyControl(int16_t item_num);

void InitialiseBaldy(int16_t item_num);
void BaldyControl(int16_t item_num);

void T1MInjectGamePeople();

#endif
