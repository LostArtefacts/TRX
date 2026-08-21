#include <trx/core/log.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/objects/general/pickup.h>
#include <trx/game/savegame.h>
#include <trx/game/stats.h>

// The bit a secret number stands for in the level's mask, or 0 when the level
// holds no such secret.
static uint32_t M_GetSecretMask(
    const GF_LEVEL *const level, const int16_t secret_idx)
{
    if (secret_idx < 0 || secret_idx >= STATS_MAX_SECRETS
        || !Stats_HasLevelMaxStats(level)) {
        return 0;
    }
    const uint32_t secret_mask = 1 << secret_idx;
    return (secret_mask & Stats_GetLevelMaxStats(level)->all_secrets_mask) != 0
        ? secret_mask
        : 0;
}

static void M_SumStats(STATS_COMMON *const dst, const STATS_COMMON *const src)
{
    for (int32_t i = 0; i < STATS_CAT_NUMBER_OF; i++) {
        dst->counts[i] += src->counts[i];
    }
    dst->timer += src->timer;
    dst->ammo_hits += src->ammo_hits;
    dst->ammo_used += src->ammo_used;
    dst->medipacks_used += src->medipacks_used;
    dst->distance_travelled += src->distance_travelled;
    dst->death_count += src->death_count;
}

static void M_SumMaxStats(
    LEVEL_MAX_STATS *const dst, const LEVEL_MAX_STATS *const src)
{
    for (int32_t i = 0; i < STATS_CAT_NUMBER_OF; i++) {
        dst->maxes[i] += src->maxes[i];
    }
    dst->max_kill_ally_count += src->max_kill_ally_count;
    dst->max_kill_non_ally_count += src->max_kill_non_ally_count;
    dst->max_pickup_secret_count += src->max_pickup_secret_count;
}

// What the game flow declares out of reach. Crystals have no such declaration.
static uint32_t M_GetCategoryUnobtainable(
    const GF_LEVEL *const level, const STATS_CATEGORY_ID id)
{
    switch (id) {
    case STATS_CAT_PICKUPS:
        return level->unobtainable.pickups;
    case STATS_CAT_KILLS:
        return level->unobtainable.kills + level->unobtainable.ally_kills;
    case STATS_CAT_SECRETS:
        return level->unobtainable.secrets;
    case STATS_CAT_CRYSTALS:
    case STATS_CAT_NUMBER_OF:
        break;
    }
    return 0;
}

LEVEL_STATS *Stats_GetLevelStats(const GF_LEVEL *const level)
{
    RESUME_INFO *const resume = SG_Resume_GetEntry(level);
    return resume == nullptr ? nullptr : &resume->stats;
}

void Stats_ResetLevel(const GF_LEVEL *const level)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(level);
    if (stats == nullptr) {
        return;
    }
    const int32_t death_count = stats->death_count;
    *stats = (LEVEL_STATS) {};
    stats->death_count = death_count;
}

bool Stats_GetCategory(
    const GF_LEVEL *const level, const STATS_CATEGORY_ID id,
    STATS_CATEGORY *const out)
{
    const LEVEL_STATS *const stats = Stats_GetLevelStats(level);
    if (stats == nullptr || !Stats_HasLevelMaxStats(level)
        || id >= STATS_CAT_NUMBER_OF) {
        return false;
    }

    const uint32_t unobtainable = M_GetCategoryUnobtainable(level, id);
    const uint32_t max = Stats_GetLevelMaxStats(level)->maxes[id];
    *out = (STATS_CATEGORY) {
        .level = level,
        .id = id,
        .count = stats->counts[id],
        .max = max,
        .raw = max + unobtainable,
        .unobtainable = unobtainable,
    };
    return true;
}

bool Stats_SetCategoryCount(
    const GF_LEVEL *const level, const STATS_CATEGORY_ID id,
    const uint32_t count)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(level);
    if (stats == nullptr || id >= STATS_CAT_NUMBER_OF
        || id == STATS_CAT_SECRETS) {
        return false;
    }
    stats->counts[id] = count;
    return true;
}

bool Stats_IsSecretValid(const GF_LEVEL *const level, const int16_t secret_idx)
{
    return M_GetSecretMask(level, secret_idx) != 0;
}

bool Stats_HasSecret(const GF_LEVEL *const level, const int16_t secret_idx)
{
    const uint32_t secret_mask = M_GetSecretMask(level, secret_idx);
    const LEVEL_STATS *const stats = Stats_GetLevelStats(level);
    if (secret_mask == 0 || stats == nullptr) {
        return false;
    }
    return (stats->secret_flags & secret_mask) != 0;
}

bool Stats_RemoveSecret(const GF_LEVEL *const level, const int16_t secret_idx)
{
    const uint32_t secret_mask = M_GetSecretMask(level, secret_idx);
    LEVEL_STATS *const stats = Stats_GetLevelStats(level);
    if (secret_mask == 0 || stats == nullptr
        || (stats->secret_flags & secret_mask) == 0) {
        return false;
    }
    LOG_INFO("Removing secret %d", secret_idx);
    stats->secret_flags &= ~secret_mask;
    stats->counts[STATS_CAT_SECRETS]--;
    return true;
}

bool Stats_AddSecret(const GF_LEVEL *const level, const int16_t secret_idx)
{
    const uint32_t secret_mask = M_GetSecretMask(level, secret_idx);
    LEVEL_STATS *const stats = Stats_GetLevelStats(level);
    if (secret_mask == 0 || stats == nullptr
        || (stats->secret_flags & secret_mask) != 0) {
        return false;
    }
    LOG_INFO("Adding secret %d", secret_idx);
    stats->secret_flags |= secret_mask;
    stats->counts[STATS_CAT_SECRETS]++;
    return true;
}

void Stats_UpdateSecrets(LEVEL_STATS *const stats)
{
    stats->counts[STATS_CAT_SECRETS] = 0;
    for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
        stats->counts[STATS_CAT_SECRETS] +=
            (stats->secret_flags & (1 << i)) ? 1 : 0;
    }
}

void Stats_MarkSecretCollected(const ITEM *const item)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(Game_GetCurrentLevel());
    stats->secret_flags |= Pickup_GetSecretMask(item);
    Stats_UpdateSecrets(stats);
}

bool Stats_CheckAllLevelSecretsCollected(void)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    const LEVEL_MAX_STATS *const max_stats = Stats_GetLevelMaxStats(level);
    const LEVEL_STATS *const stats = Stats_GetLevelStats(level);
    return stats->counts[STATS_CAT_SECRETS]
        >= max_stats->maxes[STATS_CAT_SECRETS];
}

bool Stats_CheckAllSecretsCollected(void)
{
    const FINAL_STATS final_stats = Stats_ComputeFinalStats(false);
    return final_stats.stats.counts[STATS_CAT_SECRETS]
        >= final_stats.max_stats.maxes[STATS_CAT_SECRETS];
}

void Stats_AddMedipacksUsed(const double medipack_value)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(Game_GetCurrentLevel());
    if (stats != nullptr) {
        stats->medipacks_used += medipack_value;
    }
}

void Stats_AddDeath(void)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(Game_GetCurrentLevel());
    if (stats == nullptr) {
        return;
    }
    stats->death_count++;
    const SAVEGAME_SLOT_REF save_slot = SG_Manager_GetBoundSlot();
    if (SG_Manager_IsValidSlotRef(save_slot)) {
        SHOULD(Savegame_UpdateDeathCounters(save_slot, stats->death_count));
    }
}

void Stats_UpdateTimer(void)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(Game_GetCurrentLevel());
    if (stats != nullptr) {
        stats->timer++;
    }
}

void Stats_AddKill(void)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(Game_GetCurrentLevel());
    if (stats != nullptr) {
        stats->counts[STATS_CAT_KILLS]++;
    }
}

void Stats_AddCrystal(void)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(Game_GetCurrentLevel());
    if (stats != nullptr) {
        stats->counts[STATS_CAT_CRYSTALS]++;
    }
}

void Stats_AddPickup(void)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(Game_GetCurrentLevel());
    if (stats != nullptr) {
        stats->counts[STATS_CAT_PICKUPS]++;
    }
}

void Stats_AddAmmoHits(void)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(Game_GetCurrentLevel());
    if (stats != nullptr) {
        stats->ammo_hits++;
    }
}

void Stats_AddAmmoUsed(void)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(Game_GetCurrentLevel());
    if (stats != nullptr) {
        stats->ammo_used++;
    }
}

void Stats_AddDistanceTravelled(const XYZ_32 pos, const XYZ_32 last_pos)
{
    LEVEL_STATS *const stats = Stats_GetLevelStats(Game_GetCurrentLevel());
    if (stats != nullptr) {
        stats->distance_travelled += XYZ_32_GetDistance(pos, last_pos);
    }
}

FINAL_STATS Stats_ComputeFinalStats(const bool include_bonus_levels)
{
    FINAL_STATS result = {};
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (!(level->type == GFL_NORMAL
              || (level->type == GFL_BONUS && include_bonus_levels))) {
            continue;
        }

        const LEVEL_STATS *const stats = Stats_GetLevelStats(level);
        if (stats != nullptr) {
            M_SumStats(&result.stats, (const STATS_COMMON *)stats);
        }
        if (Stats_HasLevelMaxStats(level)) {
            M_SumMaxStats(&result.max_stats, Stats_GetLevelMaxStats(level));
        }
    }

    return result;
}

OBJECT_ID Stats_GetSecretObject(
    const GF_LEVEL *const level, const int32_t secret_idx)
{
    if (secret_idx < 0 || secret_idx >= STATS_MAX_SECRETS
        || !Stats_HasLevelMaxStats(level)) {
        return NO_OBJECT;
    }
    return Stats_GetLevelMaxStats(level)
        ->secret_objects[secret_idx]
        .assigned_object_id;
}

uint32_t Stats_GetSecretMaskForItem(
    const GF_LEVEL *const level, const int16_t item_num)
{
    if (!Stats_HasLevelMaxStats(level)) {
        return 0;
    }

    const LEVEL_MAX_STATS *const max_stats = Stats_GetLevelMaxStats(level);
    for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
        if (max_stats->secret_item_masks[i].item_num == item_num) {
            return max_stats->secret_item_masks[i].secret_mask;
        }
    }

    return 0;
}

void Stats_MarkAlliesHostile(void)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    RESUME_INFO *const resume = SG_Resume_GetEntry(level);
    resume->hurt_allies = true;
}

bool Stats_HaveAlliesBeenHurt(const GF_LEVEL *const level)
{
    const RESUME_INFO *const resume = SG_Resume_GetEntry(level);
    return resume != nullptr && resume->hurt_allies;
}
