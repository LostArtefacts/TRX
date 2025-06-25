#pragma once

#include "../collision.h"

void Lara_RefuseInteraction(void);

void Lara_Extinguish(void);
void Lara_TouchLava(void);

int16_t Lara_FloorFront(const ITEM *item, int16_t ang, int32_t dist);
extern void Lara_CatchFire(void);

void Lara_UpdateRoomToHeight(int32_t height);
int32_t Lara_GetWaterDepth(int32_t x, int32_t y, int32_t z, int16_t room_num);

// Returns true if Lara has the M16 equipped and is in either anim state: 0
// (start aim); 2 (firing); or 4 (stopping firing).
bool Lara_IsM16Active(void);
