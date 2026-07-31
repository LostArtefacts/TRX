#include <fakes/stats.h>

#include <harness/fake_calls.h>

#include <trx/game/stats.h>

static LEVEL_STATS m_Stats;
static LEVEL_MAX_STATS m_MaxStats;

// The bit a secret number stands for, or 0 when the level holds no such
// secret. The real module reads the same mask off the level's scan.
static uint32_t M_GetSecretMask(
    const GF_LEVEL *const level, const int16_t secret_idx)
{
    if (level == nullptr || secret_idx < 0 || secret_idx >= STATS_MAX_SECRETS) {
        return 0;
    }
    const uint32_t secret_mask = 1 << secret_idx;
    return (secret_mask & m_MaxStats.all_secrets_mask) != 0 ? secret_mask : 0;
}

LEVEL_STATS *Stats_GetLevelStats(const GF_LEVEL *const level)
{
    return level == nullptr ? nullptr : &m_Stats;
}

bool Stats_HasLevelMaxStats(const GF_LEVEL *const level)
{
    return level != nullptr;
}

LEVEL_MAX_STATS *Stats_GetLevelMaxStats(const GF_LEVEL *const level)
{
    return &m_MaxStats;
}

bool Stats_IsSecretValid(const GF_LEVEL *const level, const int16_t secret_idx)
{
    return M_GetSecretMask(level, secret_idx) != 0;
}

bool Stats_HasSecret(const GF_LEVEL *const level, const int16_t secret_idx)
{
    const uint32_t secret_mask = M_GetSecretMask(level, secret_idx);
    return secret_mask != 0 && (m_Stats.secret_flags & secret_mask) != 0;
}

bool Stats_AddSecret(const GF_LEVEL *const level, const int16_t secret_idx)
{
    if (Stats_HasSecret(level, secret_idx)
        || !Stats_IsSecretValid(level, secret_idx)) {
        return false;
    }
    m_Stats.secret_flags |= M_GetSecretMask(level, secret_idx);
    m_Stats.secret_count++;
    return true;
}

bool Stats_RemoveSecret(const GF_LEVEL *const level, const int16_t secret_idx)
{
    if (!Stats_HasSecret(level, secret_idx)) {
        return false;
    }
    m_Stats.secret_flags &= ~M_GetSecretMask(level, secret_idx);
    m_Stats.secret_count--;
    return true;
}

static void M_Reset(void)
{
    m_Stats = (LEVEL_STATS) {};
    m_MaxStats = (LEVEL_MAX_STATS) {};
}

FAKE_ON_RESET(M_Reset)

void FakeStats_SetSecrets(const int32_t *const nums, const int32_t count)
{
    m_Stats.secret_flags = 0;
    m_Stats.secret_count = 0;
    m_MaxStats.all_secrets_mask = 0;
    for (int32_t i = 0; i < count; i++) {
        m_MaxStats.all_secrets_mask |= 1 << (nums[i] - 1);
    }
    m_MaxStats.max_secret_count = count;
}

void FakeStats_SetFound(const int32_t num, const bool found)
{
    const uint32_t secret_mask = 1 << (num - 1);
    if (found) {
        m_Stats.secret_flags |= secret_mask;
    } else {
        m_Stats.secret_flags &= ~secret_mask;
    }
    m_Stats.secret_count = 0;
    for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
        m_Stats.secret_count += (m_Stats.secret_flags & (1 << i)) != 0 ? 1 : 0;
    }
}

void FakeStats_SetMaxSecretCount(const int32_t count)
{
    m_MaxStats.max_secret_count = count;
}
