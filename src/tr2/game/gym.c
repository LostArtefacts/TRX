#include "game/gym.h"

#include "game/game_flow.h"
#include "game/music.h"
#include "game/stats.h"
#include "global/vars.h"

static bool m_IsInventoryOpenEnabled = true;

static bool M_StoreAssaultTime(uint32_t time);

static bool M_StoreAssaultTime(const uint32_t time)
{
    ASSAULT_STATS *const stats = &g_Assault;

    int32_t insert_idx = -1;
    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (stats->best_time[i] == 0 || time < stats->best_time[i]) {
            insert_idx = i;
            break;
        }
    }
    if (insert_idx == -1) {
        return false;
    }

    for (int32_t i = MAX_ASSAULT_TIMES - 1; i > insert_idx; i--) {
        stats->best_finish[i] = stats->best_finish[i - 1];
        stats->best_time[i] = stats->best_time[i - 1];
    }

    stats->finish_count++;
    stats->best_time[insert_idx] = time;
    stats->best_finish[insert_idx] = stats->finish_count;
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

ASSAULT_STATS Gym_GetAssaultStats(void)
{
    return g_Assault;
}

void Gym_ResetAssault(void)
{
    g_IsAssaultTimerActive = false;
    g_IsAssaultTimerDisplay = false;
}

void Gym_StartAssault(void)
{
    g_SaveGame.current_stats.timer = 0;
    g_IsAssaultTimerActive = true;
    g_IsAssaultTimerDisplay = true;
    Stats_StartTimer();
}

void Gym_StopAssault(void)
{
    g_IsAssaultTimerActive = false;
    g_IsAssaultTimerDisplay = true;
}

void Gym_FinishAssault(void)
{
    if (!g_IsAssaultTimerActive) {
        return;
    }

    M_StoreAssaultTime(g_SaveGame.current_stats.timer);

    if ((int32_t)g_AssaultBestTime < 0) {
        if (g_SaveGame.current_stats.timer < 100 * FRAMES_PER_SECOND) {
            // "Gosh! That was my best time yet!"
            Music_Play(MX_GYM_HINT_15, MPM_ALWAYS);
            g_AssaultBestTime = g_SaveGame.current_stats.timer;
        } else {
            // "Congratulations! You did it! But perhaps I could've been
            // faster."
            Music_Play(MX_GYM_HINT_17, MPM_ALWAYS);
            g_AssaultBestTime = 100 * FRAMES_PER_SECOND;
        }
    } else if (g_SaveGame.current_stats.timer < g_AssaultBestTime) {
        // "Gosh! That was my best time yet!"
        Music_Play(MX_GYM_HINT_15, MPM_ALWAYS);
        g_AssaultBestTime = g_SaveGame.current_stats.timer;
    } else if (
        g_SaveGame.current_stats.timer
        < g_AssaultBestTime + 5 * FRAMES_PER_SECOND) {
        // "Almost. Perhaps another try and I might beat it."
        Music_Play(MX_GYM_HINT_16, MPM_ALWAYS);
    } else {
        // "Great. But nowhere near my best time."
        Music_Play(MX_GYM_HINT_14, MPM_ALWAYS);
    }

    g_IsAssaultTimerActive = false;
}
