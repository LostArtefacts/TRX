#include "game/gym.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/music.h"
#include "game/savegame.h"
#include "game/stats.h"
#include "global/vars.h"

static bool m_IsInventoryOpenEnabled = true;
static bool m_IsAssaultTimerDisplay = false;
static bool m_IsAssaultTimerActive = false;
static uint32_t m_AssaultBestTime = -1;
static ASSAULT_STATS m_AssaultStats = {};

static bool M_StoreAssaultTime(uint32_t time);

static bool M_StoreAssaultTime(const uint32_t time)
{
    int32_t insert_idx = -1;
    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (m_AssaultStats.best_time[i] == 0
            || time < m_AssaultStats.best_time[i]) {
            insert_idx = i;
            break;
        }
    }
    if (insert_idx == -1) {
        return false;
    }

    for (int32_t i = MAX_ASSAULT_TIMES - 1; i > insert_idx; i--) {
        m_AssaultStats.best_finish[i] = m_AssaultStats.best_finish[i - 1];
        m_AssaultStats.best_time[i] = m_AssaultStats.best_time[i - 1];
    }

    m_AssaultStats.finish_count++;
    m_AssaultStats.best_time[insert_idx] = time;
    m_AssaultStats.best_finish[insert_idx] = m_AssaultStats.finish_count;
    return true;
}

bool Gym_IsAccessible(void)
{
    return g_GameFlow.gym_enabled && GF_GetGymLevel() != nullptr;
}

void Gym_SetInventoryOpenEnabled(const bool enabled)
{
    m_IsInventoryOpenEnabled = enabled;
}

bool Gym_IsInventoryOpenEnabled(void)
{
    return m_IsInventoryOpenEnabled;
}

bool Gym_IsAssaultTimerDisplay(void)
{
    return m_IsAssaultTimerDisplay;
}

bool Gym_IsAssaultTimerActive(void)
{
    return m_IsAssaultTimerActive;
}

ASSAULT_STATS Gym_GetAssaultStats(void)
{
    return m_AssaultStats;
}

void Gym_ResetAssault(void)
{
    m_IsAssaultTimerActive = false;
    m_IsAssaultTimerDisplay = false;
}

void Gym_StartAssault(void)
{
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    resume->stats.timer = 0;
    m_IsAssaultTimerActive = true;
    m_IsAssaultTimerDisplay = true;
    Stats_StartTimer();
}

void Gym_StopAssault(void)
{
    m_IsAssaultTimerActive = false;
    m_IsAssaultTimerDisplay = true;
}

void Gym_FinishAssault(void)
{
    if (!m_IsAssaultTimerActive) {
        return;
    }

    const RESUME_INFO *const resume =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    M_StoreAssaultTime(resume->stats.timer);

    if ((int32_t)m_AssaultBestTime < 0) {
        if (resume->stats.timer < 100 * FRAMES_PER_SECOND) {
            // "Gosh! That was my best time yet!"
            Music_Play(MX_GYM_HINT_15, MPM_ALWAYS);
            m_AssaultBestTime = resume->stats.timer;
        } else {
            // "Congratulations! You did it! But perhaps I could've been
            // faster."
            Music_Play(MX_GYM_HINT_17, MPM_ALWAYS);
            m_AssaultBestTime = 100 * FRAMES_PER_SECOND;
        }
    } else if (resume->stats.timer < m_AssaultBestTime) {
        // "Gosh! That was my best time yet!"
        Music_Play(MX_GYM_HINT_15, MPM_ALWAYS);
        m_AssaultBestTime = resume->stats.timer;
    } else if (
        resume->stats.timer < m_AssaultBestTime + 5 * FRAMES_PER_SECOND) {
        // "Almost. Perhaps another try and I might beat it."
        Music_Play(MX_GYM_HINT_16, MPM_ALWAYS);
    } else {
        // "Great. But nowhere near my best time."
        Music_Play(MX_GYM_HINT_14, MPM_ALWAYS);
    }

    m_IsAssaultTimerActive = false;
}
