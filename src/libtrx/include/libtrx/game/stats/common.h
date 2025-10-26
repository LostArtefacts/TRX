#pragma once

#include "../game_flow/types.h"
#include "../math.h"
#include "./types.h"

extern void Stats_CalculateStats(void);
extern void Stats_ObserveItemsLoad(void);

extern int32_t Stats_GetMaxPickups(void);
extern int32_t Stats_GetMaxKillables(void);
extern int32_t Stats_GetMaxSecrets(void);

bool Stats_HasSecret(int16_t secret_idx);
bool Stats_IsSecretValid(int16_t secret_idx);
bool Stats_TakeSecret(int16_t secret_idx);
bool Stats_AddSecret(int16_t secret_idx);
extern uint32_t Stats_GetMaxSecretFlags(void);

void Stats_UpdateSecrets(LEVEL_STATS *stats);
extern bool Stats_CheckAllSecretsCollected(GF_LEVEL_TYPE level_type);

void Stats_AddMedipacksUsed(double medipack_value);
void Stats_AddDeath(void);
void Stats_AddDistanceTravelled(XYZ_32 pos, XYZ_32 last_pos);

extern void Stats_UpdateTimer(void);
extern void Stats_AddKill(void);
extern void Stats_AddAmmoHits(void);
extern void Stats_AddAmmoUsed(void);
