#include "game/gym.h"

#include "config.h"
#include "game/const.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/lara.h"
#include "game/music.h"
#include "game/savegame.h"
#include "game/stats.h"

#define NO_TIME (-1)

static bool m_IsInventoryOpenEnabled = true;
static bool m_IsAssaultTimerDisplay = false;
static bool m_IsAssaultTimerActive = false;
static int16_t m_CompletionTimer = 0;

static int32_t M_GetBestTime(void)
{
    const ASSAULT_STATS *const assault = &g_Config.profile.assault_stats;
    return assault->total_attempts > 0 ? (int32_t)assault->entries[0].time
                                       : NO_TIME;
}

static bool M_StoreAssaultTime(const uint32_t time)
{
    ASSAULT_STATS *const assault = &g_Config.profile.assault_stats;
    int32_t insert_idx = -1;
    for (int32_t i = 0; i < MAX_ASSAULT_TIMES; i++) {
        if (assault->entries[i].time == 0 || time < assault->entries[i].time) {
            insert_idx = i;
            break;
        }
    }
    if (insert_idx == -1) {
        return false;
    }

    for (int32_t i = MAX_ASSAULT_TIMES - 1; i > insert_idx; i--) {
        assault->entries[i] = assault->entries[i - 1];
    }

    assault->total_attempts++;
    assault->entries[insert_idx].time = time;
    assault->entries[insert_idx].attempt_num = assault->total_attempts;
    Config_Update();
    return true;
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
    return g_Config.profile.assault_stats;
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

    const int32_t current_best_time = M_GetBestTime();
    ASSAULT_STATS *const assault = &g_Config.profile.assault_stats;
    const RESUME_INFO *const resume =
        Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    M_StoreAssaultTime(resume->stats.timer);

    if (current_best_time <= 0) {
        if (resume->stats.timer < 100 * LOGIC_FPS) {
            // "Gosh! That was my best time yet!"
            Music_Play(MX_TR2_GYM_HINT_15, MPM_ALWAYS);
        } else {
            // "Congratulations! You did it! But perhaps I could've been
            // faster."
            Music_Play(MX_TR2_GYM_HINT_17, MPM_ALWAYS);
        }
    } else if (resume->stats.timer < (uint32_t)current_best_time) {
        // "Gosh! That was my best time yet!"
        Music_Play(MX_TR2_GYM_HINT_15, MPM_ALWAYS);
    } else if (
        resume->stats.timer < (uint32_t)current_best_time + 5 * LOGIC_FPS) {
        // "Almost. Perhaps another try and I might beat it."
        Music_Play(MX_TR2_GYM_HINT_16, MPM_ALWAYS);
    } else {
        // "Great. But nowhere near my best time."
        Music_Play(MX_TR2_GYM_HINT_14, MPM_ALWAYS);
    }

    m_IsAssaultTimerActive = false;
}

bool Gym_HasAssaultStats(void)
{
    return TR_VERSION >= 2;
}

bool Gym_CanPlayMusicTrack(MUSIC_ID *const track_id)
{
    const uint16_t flags = Music_GetTrackFlags(*track_id);
    const ITEM *const lara = Lara_GetItem();
    switch (Music_FromGameID(*track_id)) {
    case MX_TR1_GYM_HINT_03:
        if ((flags & IF_ONE_SHOT) != 0
            && lara->current_anim_state == LS(LS_JUMP_UP)) {
            *track_id = Music_ToGameID(MX_TR1_GYM_HINT_04);
        }
        break;

    case MX_TR1_GYM_HINT_12:
        if (lara->current_anim_state != LS(LS_HANG)) {
            return false;
        }
        break;

    case MX_TR1_GYM_HINT_16:
        if (lara->current_anim_state != LS(LS_HANG)) {
            return false;
        }
        break;

    case MX_TR1_GYM_HINT_17:
        if ((flags & IF_ONE_SHOT) != 0
            && lara->current_anim_state == LS(LS_HANG)) {
            *track_id = Music_ToGameID(MX_TR1_GYM_HINT_18);
        }
        break;

    case MX_TR1_GYM_HINT_24:
        if (lara->current_anim_state != LS(LS_SURF_TREAD)) {
            return false;
        }
        break;

    case MX_TR1_GYM_HINT_25:
        if ((flags & IF_ONE_SHOT) != 0) {
            m_CompletionTimer++;
            if (m_CompletionTimer == LOGIC_FPS * 4) {
                Game_SetIsLevelComplete(true);
                m_CompletionTimer = 0;
            }
        } else if (lara->current_anim_state != LS(LS_WATER_OUT)) {
            return false;
        }
        break;

    default:
        return true;
    }
    return true;
}
