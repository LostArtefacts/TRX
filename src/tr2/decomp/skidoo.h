#pragma once

#include "global/types.h"

#define SKIDOO_MIN_SPEED 15
#define SKIDOO_MAX_SPEED 100
#define SKIDOO_SLOW_SPEED 50
#define SKIDOO_FAST_SPEED 150

#define SKIDOO_MAX_TURN (DEG_1 * 6) // = 1092
#define SKIDOO_GUN_MESH 4

extern BITE g_Skidoo_LeftGun;
extern BITE g_Skidoo_RightGun;

void Skidoo_Initialise(int16_t item_num);
int32_t Skidoo_CheckGetOn(int16_t item_num, COLL_INFO *coll);
void Skidoo_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void Skidoo_BaddieCollision(ITEM *skidoo);
int32_t Skidoo_TestHeight(
    const ITEM *item, int32_t z_off, int32_t x_off, XYZ_32 *out_pos);
void Skidoo_DoSnowEffect(const ITEM *skidoo);
int32_t Skidoo_Dynamics(ITEM *skidoo);
int32_t Skidoo_UserControl(ITEM *skidoo, int32_t height, int32_t *out_pitch);
int32_t Skidoo_CheckGetOffOK(int32_t direction);
void Skidoo_Animation(ITEM *skidoo, int32_t collide, int32_t dead);
void Skidoo_Explode(const ITEM *skidoo);
int32_t Skidoo_CheckGetOff(void);
void Skidoo_Guns(void);
int32_t Skidoo_Control(void);
void Skidoo_Draw(const ITEM *item);
