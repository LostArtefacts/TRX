#pragma once

#include "global/types.h"

void __cdecl Camera_Initialise(void);
void __cdecl Camera_ResetPosition(void);
void __cdecl Camera_Move(const GAME_VECTOR *target, int32_t speed);
void __cdecl Camera_Clip(
    int32_t *x, int32_t *y, int32_t *h, int32_t target_x, int32_t target_y,
    int32_t target_h, int32_t left, int32_t top, int32_t right, int32_t bottom);
void __cdecl Camera_Shift(
    int32_t *x, int32_t *y, int32_t *h, int32_t target_x, int32_t target_y,
    int32_t target_h, int32_t left, int32_t top, int32_t right, int32_t bottom);
const SECTOR *__cdecl Camera_GoodPosition(
    int32_t x, int32_t y, int32_t z, int16_t room_num);
void __cdecl Camera_SmartShift(
    GAME_VECTOR *target,
    void(__cdecl *shift)(
        int32_t *x, int32_t *y, int32_t *h, int32_t target_x, int32_t target_y,
        int32_t target_h, int32_t left, int32_t top, int32_t right,
        int32_t bottom));
void __cdecl Camera_Chase(const ITEM *item);
int32_t __cdecl Camera_ShiftClamp(GAME_VECTOR *pos, int32_t clamp);
void __cdecl Camera_Combat(const ITEM *item);
void __cdecl Camera_Look(const ITEM *item);
void __cdecl Camera_Fixed(void);
void __cdecl Camera_Update(void);
void __cdecl Camera_LoadCutsceneFrame(void);
void __cdecl Camera_UpdateCutscene(void);
void __cdecl Camera_RefreshFromTrigger(int16_t type, const int16_t *fd);
