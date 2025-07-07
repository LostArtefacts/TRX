#pragma once

#include "global/types.h"

typedef enum {
    LA_G_AIM = 0,
    LA_G_DRAW = 1,
    LA_G_RECOIL = 2,
    LA_G_UNDRAW = 3,
    LA_G_UNAIM = 4,
    LA_G_RELOAD = 5,
    LA_G_UAIM = 6,
    LA_G_UUNAIM = 7,
    LA_G_URECOIL = 8,
    LA_G_SURF_UNDRAW = 9,
} LARA_GUN_ANIMATION;

void Gun_TargetInfo(const WEAPON_INFO *winfo);
void Gun_GetNewTarget(const WEAPON_INFO *winfo);
void Gun_AimWeapon(const WEAPON_INFO *winfo, LARA_ARM *arm);
void Gun_FindTargetPoint(const ITEM *item, GAME_VECTOR *target);
void Gun_HitTarget(ITEM *item, const GAME_VECTOR *hit_pos, int32_t damage);
void Gun_SmashItem(int16_t item_num, LARA_GUN_TYPE weapon_type);
void Gun_DrawFlash(LARA_GUN_TYPE weapon_type, int32_t clip);
