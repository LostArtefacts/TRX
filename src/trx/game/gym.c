#include <trx/game/gym.h>

#include <trx/config.h>
#include <trx/game/const.h>
#include <trx/game/game.h>
#include <trx/game/game_flow.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/pathing/lot.h>
#include <trx/game/savegame.h>
#include <trx/game/stats.h>
#include <trx/utils.h>
#include <trx/version.h>

#define M_NO_TIME (-1)
#define M_MAX_ASSAULT_TIME_FRAMES (60 * 60 * LOGIC_FPS - 3) // 59:59

static int32_t m_IsInventoryOpenEnabled = -1;
static bool m_IsAssaultTimerDisplay = false;
static bool m_IsAssaultTimerActive = false;
static int16_t m_CompletionTimer = 0;

static int32_t m_AssaultPenaltyDisplayTimer = 0;
static int32_t m_AssaultPenaltyFrames = 0;
static int32_t m_AssaultTargetPenaltyFrames = 0;
static int32_t m_AssaultTargetsRemaining = 0;
static int32_t m_AssaultTimerAutoHideTimer = 0;
static bool m_AssaultPadTouchedThisFrame = false;
static bool m_AssaultPadLock = false;

static void M_ResetAssaultTargets(void)
{
    m_AssaultTargetsRemaining = 0;

    const OBJECT *const obj = Object_Get(O_ASSAULT_TARGET);
    if (!obj->loaded) {
        return;
    }

    for (int16_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (item->object_id != O_ASSAULT_TARGET) {
            continue;
        }

        // Ensure we don't run out of LOT slots after repeated restarts.
        if (item->data != nullptr) {
            LOT_DisableBaddieAI(item_num);
        }

        if (obj->initialise_func != nullptr) {
            obj->initialise_func(item_num);
        }

        m_AssaultTargetsRemaining++;
    }
}

static int32_t M_GetBestTime(void)
{
    const ASSAULT_STATS *const assault = &g_Config.profile.assault_stats;
    return assault->total_attempts > 0 ? (int32_t)assault->entries[0].time
                                       : M_NO_TIME;
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
    if (m_IsInventoryOpenEnabled == -1) {
        m_IsInventoryOpenEnabled = g_TRVersion >= 2;
    }
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

void Gym_Control(void)
{
    if (g_TRVersion < 3) {
        return;
    }

    if (m_AssaultPadLock && !m_AssaultPadTouchedThisFrame) {
        m_AssaultPadLock = false;
    }
    m_AssaultPadTouchedThisFrame = false;

    if (m_AssaultPenaltyDisplayTimer > 0) {
        m_AssaultPenaltyDisplayTimer--;
    }
    if (!m_IsAssaultTimerActive && m_IsAssaultTimerDisplay
        && m_AssaultTimerAutoHideTimer > 0) {
        m_AssaultTimerAutoHideTimer--;
        if (m_AssaultTimerAutoHideTimer == 0) {
            m_IsAssaultTimerDisplay = false;
        }
    }
}

void Gym_Assault_AddPenaltySeconds(const int32_t seconds)
{
    if (g_TRVersion < 3) {
        return;
    }
    if (!m_IsAssaultTimerActive) {
        return;
    }
    if (seconds <= 0) {
        return;
    }

    m_AssaultPenaltyDisplayTimer = 4 * LOGIC_FPS;
    m_AssaultPenaltyFrames += seconds * LOGIC_FPS;
    CLAMPG(m_AssaultPenaltyFrames, M_MAX_ASSAULT_TIME_FRAMES);
}

void Gym_Assault_DecreaseTargetCount(void)
{
    if (m_AssaultTargetsRemaining > 0) {
        m_AssaultTargetsRemaining--;
    }
}

int32_t Gym_Assault_GetPenaltyDisplayTimer(void)
{
    return m_AssaultPenaltyDisplayTimer;
}

int32_t Gym_Assault_GetPenaltyFrames(void)
{
    return m_AssaultPenaltyFrames;
}

int32_t Gym_Assault_GetTargetPenaltyFrames(void)
{
    return m_AssaultTargetPenaltyFrames;
}

bool Gym_Assault_OnPadContact(const bool on_ground)
{
    if (g_TRVersion < 3) {
        return true;
    }
    if (!Game_IsInGym()) {
        return true;
    }

    m_AssaultPadTouchedThisFrame = true;
    if (!on_ground) {
        return true;
    }
    if (m_AssaultPadLock) {
        return false;
    }

    m_AssaultPadLock = true;
    return true;
}

void Gym_ResetAssault(void)
{
    m_IsAssaultTimerActive = false;
    m_IsAssaultTimerDisplay = false;
    m_AssaultPenaltyFrames = 0;
    m_AssaultTargetPenaltyFrames = 0;
    m_AssaultPenaltyDisplayTimer = 0;
    m_AssaultTargetsRemaining = 0;
    m_AssaultTimerAutoHideTimer = 0;
    m_AssaultPadTouchedThisFrame = false;
    m_AssaultPadLock = false;
}

void Gym_StartAssault(void)
{
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    resume->stats.timer = 0;
    m_IsAssaultTimerActive = true;
    m_IsAssaultTimerDisplay = true;
    m_AssaultPenaltyFrames = 0;
    m_AssaultTargetPenaltyFrames = 0;
    m_AssaultPenaltyDisplayTimer = 0;
    m_AssaultTimerAutoHideTimer = 0;
    m_AssaultPadTouchedThisFrame = false;
    m_AssaultPadLock = false;
    M_ResetAssaultTargets();
}

void Gym_StopAssault(void)
{
    m_IsAssaultTimerActive = false;
    m_IsAssaultTimerDisplay = true;
    m_AssaultTimerAutoHideTimer = 0;
}

void Gym_FinishAssault(void)
{
    if (!m_IsAssaultTimerActive) {
        return;
    }

    RESUME_INFO *const resume = Savegame_GetCurrentInfo(Game_GetCurrentLevel());

    uint32_t final_time = resume->stats.timer;
    if (g_TRVersion >= 3) {
        m_AssaultPenaltyDisplayTimer = 10 * LOGIC_FPS;
        m_AssaultTargetPenaltyFrames =
            10 * LOGIC_FPS * m_AssaultTargetsRemaining;
        CLAMPG(m_AssaultTargetPenaltyFrames, M_MAX_ASSAULT_TIME_FRAMES);

        final_time += (uint32_t)m_AssaultPenaltyFrames
            + (uint32_t)m_AssaultTargetPenaltyFrames;
        CLAMPG(final_time, M_MAX_ASSAULT_TIME_FRAMES);
        resume->stats.timer = final_time;
    }

    M_StoreAssaultTime(final_time);

    if (g_TRVersion >= 3) {
        if (final_time < (uint32_t)(180 * LOGIC_FPS)) {
            Music_Play(MX_TR3_GYM_HINT_FAST_TIME, MPM_ONCE);
        }
        m_AssaultTimerAutoHideTimer = 15 * LOGIC_FPS;
    } else {
        const int32_t current_best_time = M_GetBestTime();
        if (current_best_time <= 0) {
            if (final_time < (uint32_t)(100 * LOGIC_FPS)) {
                // "Gosh! That was my best time yet!"
                Music_Play(MX_TR2_GYM_HINT_15, MPM_ONCE);
            } else {
                // "Congratulations! You did it! But perhaps I could've been
                // faster."
                Music_Play(MX_TR2_GYM_HINT_17, MPM_ONCE);
            }
        } else if (final_time < (uint32_t)current_best_time) {
            // "Gosh! That was my best time yet!"
            Music_Play(MX_TR2_GYM_HINT_15, MPM_ONCE);
        } else if (final_time < (uint32_t)current_best_time + 5 * LOGIC_FPS) {
            // "Almost. Perhaps another try and I might beat it."
            Music_Play(MX_TR2_GYM_HINT_16, MPM_ONCE);
        } else {
            // "Great. But nowhere near my best time."
            Music_Play(MX_TR2_GYM_HINT_14, MPM_ONCE);
        }
    }

    m_IsAssaultTimerActive = false;
}

bool Gym_HasAssaultStats(void)
{
    return g_TRVersion >= 2;
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
