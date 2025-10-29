#pragma once

#include "../game_flow/types.h"
#include "../math.h"
#include "./types.h"

bool Stats_HasSecret(int16_t secret_idx);
bool Stats_RemoveSecret(int16_t secret_idx);
bool Stats_AddSecret(int16_t secret_idx);
bool Stats_IsSecretValid(int16_t secret_idx);
OBJECT_ID Stats_GetSecretObject(int32_t secret_idx);

void Stats_UpdateSecrets(LEVEL_STATS *stats);
void Stats_MarkSecretCollected(const ITEM *item);
bool Stats_CheckAllSecretsCollected(GF_LEVEL_TYPE level_type);
bool Stats_CheckAllLevelSecretsPickedUp(void);

void Stats_UpdateTimer(void);
void Stats_AddKill(void);
void Stats_AddPickup(void);
void Stats_AddAmmoHits(void);
void Stats_AddAmmoUsed(void);
void Stats_AddDeath(void);
void Stats_AddMedipacksUsed(double medipack_value);
void Stats_AddDistanceTravelled(XYZ_32 pos, XYZ_32 last_pos);

FINAL_STATS Stats_ComputeFinalStats(GF_LEVEL_TYPE level_type);
