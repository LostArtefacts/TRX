#include <fakes/stats.h>

#include <harness/fake_calls.h>

#include <trx/game/game/state.h>
#include <trx/game/game_flow.h>
#include <trx/game/stats.h>

// One entry per level, addressed by the level's place in its table, which is
// enough for a game flow the tests keep small.
#define M_LEVEL_SLOTS 16

static LEVEL_STATS m_Stats[M_LEVEL_SLOTS];
static LEVEL_MAX_STATS m_MaxStats[M_LEVEL_SLOTS];
static bool m_AlliesHurt[M_LEVEL_SLOTS];

// Where a level's counters sit, or -1 for a level that counts nothing.
static int32_t M_GetSlot(const GF_LEVEL *const level)
{
    if (level == nullptr || level->type == GFL_TITLE
        || level->type == GFL_CUTSCENE || level->num < 0
        || level->num >= M_LEVEL_SLOTS) {
        return -1;
    }
    return level->num;
}

// The bit a secret number stands for, or 0 when the level holds no such
// secret. The real module reads the same mask off the level's scan.
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

static uint32_t M_GetUnobtainable(
    const GF_LEVEL *const level, const STATS_CATEGORY_ID id)
{
    switch (id) {
    case STATS_CAT_PICKUPS:
        return level->unobtainable.pickups;
    case STATS_CAT_KILLS:
        return level->unobtainable.kills + level->unobtainable.ally_kills;
    case STATS_CAT_SECRETS:
        return level->unobtainable.secrets;
    default:
        return 0;
    }
}

static void M_Reset(void)
{
    for (int32_t i = 0; i < M_LEVEL_SLOTS; i++) {
        m_Stats[i] = (LEVEL_STATS) {};
        m_MaxStats[i] = (LEVEL_MAX_STATS) {};
        m_AlliesHurt[i] = false;
    }
}

LEVEL_STATS *Stats_GetLevelStats(const GF_LEVEL *const level)
{
    const int32_t slot = M_GetSlot(level);
    return slot < 0 ? nullptr : &m_Stats[slot];
}

bool Stats_HasLevelMaxStats(const GF_LEVEL *const level)
{
    return M_GetSlot(level) >= 0;
}

LEVEL_MAX_STATS *Stats_GetLevelMaxStats(const GF_LEVEL *const level)
{
    const int32_t slot = M_GetSlot(level);
    return slot < 0 ? nullptr : &m_MaxStats[slot];
}

bool Stats_GetCategory(
    const GF_LEVEL *const level, const STATS_CATEGORY_ID id,
    STATS_CATEGORY *const out)
{
    const LEVEL_STATS *const stats = Stats_GetLevelStats(level);
    if (stats == nullptr || id >= STATS_CAT_NUMBER_OF) {
        return false;
    }
    const uint32_t unobtainable = M_GetUnobtainable(level, id);
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

bool Stats_AddSecret(const GF_LEVEL *const level, const int16_t secret_idx)
{
    const uint32_t secret_mask = M_GetSecretMask(level, secret_idx);
    LEVEL_STATS *const stats = Stats_GetLevelStats(level);
    if (secret_mask == 0 || stats == nullptr
        || (stats->secret_flags & secret_mask) != 0) {
        return false;
    }
    stats->secret_flags |= secret_mask;
    stats->counts[STATS_CAT_SECRETS]++;
    return true;
}

bool Stats_RemoveSecret(const GF_LEVEL *const level, const int16_t secret_idx)
{
    const uint32_t secret_mask = M_GetSecretMask(level, secret_idx);
    LEVEL_STATS *const stats = Stats_GetLevelStats(level);
    if (secret_mask == 0 || stats == nullptr
        || (stats->secret_flags & secret_mask) == 0) {
        return false;
    }
    stats->secret_flags &= ~secret_mask;
    stats->counts[STATS_CAT_SECRETS]--;
    return true;
}

bool Stats_HaveAlliesBeenHurt(const GF_LEVEL *const level)
{
    const int32_t slot = M_GetSlot(level);
    return slot >= 0 && m_AlliesHurt[slot];
}

FAKE_ON_RESET(M_Reset)

void FakeStats_SetSecrets(const int32_t *const nums, const int32_t count)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    LEVEL_STATS *const stats = Stats_GetLevelStats(level);
    LEVEL_MAX_STATS *const max_stats = Stats_GetLevelMaxStats(level);
    stats->secret_flags = 0;
    stats->counts[STATS_CAT_SECRETS] = 0;
    max_stats->all_secrets_mask = 0;
    for (int32_t i = 0; i < count; i++) {
        max_stats->all_secrets_mask |= 1 << (nums[i] - 1);
    }
    max_stats->maxes[STATS_CAT_SECRETS] = count;
}

void FakeStats_SetFound(const int32_t num, const bool found)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    if (found) {
        Stats_AddSecret(level, num - 1);
    } else {
        Stats_RemoveSecret(level, num - 1);
    }
}

void FakeStats_SetMaxSecretCount(const int32_t count)
{
    Stats_GetLevelMaxStats(Game_GetCurrentLevel())->maxes[STATS_CAT_SECRETS] =
        count;
}

void FakeStats_SetCount(
    const int32_t level_num, const int32_t id, const int32_t count)
{
    Stats_GetLevelStats(GF_GetLevelByOrdinalNumber(GFLT_MAIN, level_num))
        ->counts[id] = count;
}

void FakeStats_SetMax(
    const int32_t level_num, const int32_t id, const int32_t max)
{
    Stats_GetLevelMaxStats(GF_GetLevelByOrdinalNumber(GFLT_MAIN, level_num))
        ->maxes[id] = max;
}

void FakeStats_SetUnobtainable(
    const int32_t level_num, const int32_t id, const int32_t count)
{
    GF_LEVEL *const level = GF_GetLevelByOrdinalNumber(GFLT_MAIN, level_num);
    switch (id) {
    case STATS_CAT_PICKUPS:
        level->unobtainable.pickups = count;
        break;
    case STATS_CAT_KILLS:
        level->unobtainable.kills = count;
        break;
    case STATS_CAT_SECRETS:
        level->unobtainable.secrets = count;
        break;
    default:
        break;
    }
}

void FakeStats_SetKillSplit(
    const int32_t level_num, const int32_t allies, const int32_t enemies)
{
    LEVEL_MAX_STATS *const max_stats = Stats_GetLevelMaxStats(
        GF_GetLevelByOrdinalNumber(GFLT_MAIN, level_num));
    max_stats->max_kill_ally_count = allies;
    max_stats->max_kill_non_ally_count = enemies;
    max_stats->maxes[STATS_CAT_KILLS] = allies + enemies;
}

void FakeStats_SetAlliesHurt(const int32_t level_num, const bool hurt)
{
    m_AlliesHurt[GF_GetLevelByOrdinalNumber(GFLT_MAIN, level_num)->num] = hurt;
}
