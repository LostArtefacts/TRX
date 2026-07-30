#include <fakes/stats.h>

#include <harness/fake_calls.h>

#include <trx/game/stats.h>

static bool m_Valid[STATS_MAX_SECRETS];
static bool m_Found[STATS_MAX_SECRETS];
static int32_t m_MaxSecretCount;

static bool M_InRange(const int16_t secret_idx)
{
    return secret_idx >= 0 && secret_idx < STATS_MAX_SECRETS;
}

bool Stats_IsSecretValid(const int16_t secret_idx)
{
    return M_InRange(secret_idx) && m_Valid[secret_idx];
}

bool Stats_HasSecret(const int16_t secret_idx)
{
    return Stats_IsSecretValid(secret_idx) && m_Found[secret_idx];
}

bool Stats_AddSecret(const int16_t secret_idx)
{
    if (!Stats_IsSecretValid(secret_idx) || m_Found[secret_idx]) {
        return false;
    }
    m_Found[secret_idx] = true;
    return true;
}

bool Stats_RemoveSecret(const int16_t secret_idx)
{
    if (!Stats_HasSecret(secret_idx)) {
        return false;
    }
    m_Found[secret_idx] = false;
    return true;
}

int32_t Stats_GetSecretCount(void)
{
    int32_t count = 0;
    for (int16_t i = 0; i < STATS_MAX_SECRETS; i++) {
        count += Stats_HasSecret(i) ? 1 : 0;
    }
    return count;
}

int32_t Stats_GetMaxSecretCount(void)
{
    return m_MaxSecretCount;
}

static void M_Reset(void)
{
    for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
        m_Valid[i] = false;
        m_Found[i] = false;
    }
    m_MaxSecretCount = 0;
}

FAKE_ON_RESET(M_Reset)

void FakeStats_SetSecrets(const int32_t *const nums, const int32_t count)
{
    for (int32_t i = 0; i < STATS_MAX_SECRETS; i++) {
        m_Valid[i] = false;
        m_Found[i] = false;
    }
    for (int32_t i = 0; i < count; i++) {
        m_Valid[nums[i] - 1] = true;
    }
    m_MaxSecretCount = count;
}

void FakeStats_SetFound(const int32_t num, const bool found)
{
    m_Found[num - 1] = found;
}

void FakeStats_SetMaxSecretCount(const int32_t count)
{
    m_MaxSecretCount = count;
}
