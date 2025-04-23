#pragma once

#include "global/types.h"

#include <libtrx/game/game_flow.h>
#include <libtrx/game/stats.h>

void Stats_UpdateTimer(void);
void Stats_CalculateStats(void);
int32_t Stats_GetSecrets(void);
void Stats_MarkSecretCollected(GAME_OBJECT_ID obj_id);
bool Stats_CheckAllLevelSecretsCollected(void);
FINAL_STATS Stats_ComputeFinalStats(GF_LEVEL_TYPE level_type);
bool Stats_CheckAllSecretsCollected(GF_LEVEL_TYPE level_type);

void Stats_AddKill(void);
void Stats_AddAmmoHits(void);
void Stats_AddAmmoUsed(void);
void Stats_AddMedipacksUsed(double medipack_value);
void Stats_AddDistanceTravelled(XYZ_32 pos, XYZ_32 last_pos);
