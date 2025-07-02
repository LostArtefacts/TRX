#pragma once

#include "./types.h"

extern void Stats_StartTimer(void);
extern void Stats_ObserveItemsLoad(void);

bool Stats_HasSecret(int16_t secret_idx);
bool Stats_IsSecretValid(int16_t secret_idx);
bool Stats_TakeSecret(int16_t secret_idx);
bool Stats_AddSecret(int16_t secret_idx);
extern uint32_t Stats_GetMaxSecretFlags(void);

void Stats_UpdateSecrets(LEVEL_STATS *stats);
