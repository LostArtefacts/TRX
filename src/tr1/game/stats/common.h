#pragma once

#include "global/types.h"

#include <libtrx/game/stats.h>

void Stats_ObserveRoomsLoad(void);
void Stats_CalculateStats(void);
int32_t Stats_GetPickups(void);
int32_t Stats_GetKillables(void);
int32_t Stats_GetSecrets(void);
void Stats_ComputeFinal(GF_LEVEL_TYPE level_type, FINAL_STATS *stats);
bool Stats_CheckAllSecretsCollected(GF_LEVEL_TYPE level_type);

void Stats_UpdateTimer(void);
void Stats_UpdateSecrets(LEVEL_STATS *stats);

void Stats_AddKill(void);
bool Stats_AddSecret(int16_t secret_number);
void Stats_AddPickup(void);
void Stats_AddAmmoHits(void);
void Stats_AddAmmoUsed(void);
void Stats_AddMedipacksUsed(double medipack_value);
void Stats_AddDistanceTravelled(XYZ_32 pos, XYZ_32 last_pos);
void Stats_AddDeath(void);
