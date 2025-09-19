#pragma once

#include "../math.h"
#include "./types.h"

extern void Stats_ObserveItemsLoad(void);

bool Stats_HasSecret(int16_t secret_idx);
bool Stats_IsSecretValid(int16_t secret_idx);
bool Stats_TakeSecret(int16_t secret_idx);
bool Stats_AddSecret(int16_t secret_idx);
extern uint32_t Stats_GetMaxSecretFlags(void);

void Stats_UpdateSecrets(LEVEL_STATS *stats);

void Stats_AddMedipacksUsed(double medipack_value);
void Stats_AddDeath(void);
void Stats_AddDistanceTravelled(XYZ_32 pos, XYZ_32 last_pos);

extern void Stats_AddKill(void);
extern void Stats_AddAmmoHits(void);
extern void Stats_AddAmmoUsed(void);
