#pragma once

#include "global/types.h"

typedef enum {
    LF_G_AIM_START = 0,
    LF_G_AIM_BEND = 1,
    LF_G_AIM_EXTEND = 3,
    LF_G_AIM_END = 4,
    LF_G_UNDRAW_START = 5,
    LF_G_UNDRAW_BEND = 6,
    LF_G_UNDRAW_END = 12,
    LF_G_DRAW_START = 13,
    LF_G_DRAW_END = 23,
    LF_G_RECOIL_START = 24,
    LF_G_RECOIL_END = 32,
} LARA_GUN_ANIMATION_FRAME;

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

int32_t __cdecl Gun_CheckForHoldingState(const LARA_STATE state);
void __cdecl Gun_TargetInfo(const WEAPON_INFO *winfo);
void __cdecl Gun_GetNewTarget(const WEAPON_INFO *winfo);
void __cdecl Gun_AimWeapon(const WEAPON_INFO *winfo, LARA_ARM *arm);
int32_t __cdecl Gun_FireWeapon(
    LARA_GUN_TYPE weapon_type, ITEM *target, const ITEM *src,
    const PHD_ANGLE *angles);
void __cdecl Gun_FindTargetPoint(const ITEM *item, GAME_VECTOR *target);
void __cdecl Gun_HitTarget(
    ITEM *item, const GAME_VECTOR *hit_pos, int32_t damage);
void __cdecl Gun_SmashItem(int16_t item_num, LARA_GUN_TYPE weapon_type);
