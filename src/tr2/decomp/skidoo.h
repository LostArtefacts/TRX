#pragma once

#include "global/types.h"

void __cdecl Skidoo_Initialise(int16_t item_num);
int32_t __cdecl Skidoo_CheckGetOn(int16_t item_num, COLL_INFO *coll);
void __cdecl Skidoo_Collision(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void __cdecl Skidoo_BaddieCollision(const ITEM *skidoo);
int32_t __cdecl Skidoo_TestHeight(
    const ITEM *item, int32_t z_off, int32_t x_off, XYZ_32 *out_pos);
void __cdecl Skidoo_DoSnowEffect(ITEM *skidoo);
int32_t __cdecl Skidoo_Dynamics(ITEM *const skidoo);
int32_t __cdecl Skidoo_UserControl(
    ITEM *skidoo, int32_t height, int32_t *out_pitch);
