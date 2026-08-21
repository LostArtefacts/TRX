#pragma once

#include <trx/core/math.h>
#include <trx/game/game_flow/types.h>
#include <trx/game/stats/types.h>

// The counters the level keeps, or nullptr where there are none to keep: the
// title screen and the cutscenes.
LEVEL_STATS *Stats_GetLevelStats(const GF_LEVEL *level);

// Clears what the level counts, ready for it to be played. The death count
// stands apart and is left alone: a death belongs to the level it happened on
// however many times the level is entered.
void Stats_ResetLevel(const GF_LEVEL *level);

// What the level is counted on in one category, or false where there is
// nothing to count: a level with no scan behind it.
bool Stats_GetCategory(
    const GF_LEVEL *level, STATS_CATEGORY_ID id, STATS_CATEGORY *out);

// Puts a count back. False where the category cannot take one: the secrets are
// held as a mask and go through the secret verbs, so that the two agree.
bool Stats_SetCategoryCount(
    const GF_LEVEL *level, STATS_CATEGORY_ID id, uint32_t count);

bool Stats_HasSecret(const GF_LEVEL *level, int16_t secret_idx);
bool Stats_RemoveSecret(const GF_LEVEL *level, int16_t secret_idx);
bool Stats_AddSecret(const GF_LEVEL *level, int16_t secret_idx);
bool Stats_IsSecretValid(const GF_LEVEL *level, int16_t secret_idx);
OBJECT_ID Stats_GetSecretObject(const GF_LEVEL *level, int32_t secret_idx);
uint32_t Stats_GetSecretMaskForItem(const GF_LEVEL *level, int16_t item_num);

void Stats_UpdateSecrets(LEVEL_STATS *stats);
void Stats_MarkSecretCollected(const ITEM *item);
bool Stats_CheckAllSecretsCollected(void);
bool Stats_CheckAllLevelSecretsCollected(void);

void Stats_UpdateTimer(void);
void Stats_AddKill(void);
void Stats_AddCrystal(void);
void Stats_AddPickup(void);
void Stats_AddAmmoHits(void);
void Stats_AddAmmoUsed(void);
void Stats_AddDeath(void);
void Stats_AddMedipacksUsed(double medipack_value);
void Stats_AddDistanceTravelled(XYZ_32 pos, XYZ_32 last_pos);
void Stats_MarkAlliesHostile(void);

// Whether Lara has hurt an ally in this level. The kill maximum only holds the
// allies against her once she has.
bool Stats_HaveAlliesBeenHurt(const GF_LEVEL *level);

FINAL_STATS Stats_ComputeFinalStats(bool include_bonus_levels);
