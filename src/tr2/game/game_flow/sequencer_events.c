#include "game/savegame.h"
#include "game/stats.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/game.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/lua.h>
#include <libtrx/game/music.h>
#include <libtrx/game/option/passport.h>
#include <libtrx/game/output.h>

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayMusic);
static DECLARE_GF_EVENT_HANDLER(M_HandleLevelComplete);
static DECLARE_GF_EVENT_HANDLER(M_HandleEnableSunset);
static DECLARE_GF_EVENT_HANDLER(M_HandleDisableFloor);
static DECLARE_GF_EVENT_HANDLER(M_HandleSetStartAnim);

static DECLARE_GF_EVENT_HANDLER((*m_EventHandlers[GFS_NUMBER_OF])) = {
    // clang-format off
    [GFS_PLAY_MUSIC]       = M_HandlePlayMusic,
    [GFS_LEVEL_COMPLETE]   = M_HandleLevelComplete,
    [GFS_ENABLE_SUNSET]    = M_HandleEnableSunset,
    [GFS_DISABLE_FLOOR]    = M_HandleDisableFloor,
    [GFS_SET_START_ANIM]   = M_HandleSetStartAnim,
    // clang-format on
};

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayMusic)
{
    if (seq_ctx != GFSC_STORY) {
        Music_Play_Direct((int32_t)(intptr_t)event->data, MPM_ALWAYS);
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleLevelComplete)
{
    if (seq_ctx != GFSC_NORMAL) {
        return (GF_COMMAND) { .action = GF_NOOP };
    }
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();
    const GF_LEVEL *const next_level = GF_GetLevelAfter(current_level);

    if (current_level == GF_GetLastLevel()) {
        g_Config.profile.new_game_plus_unlock = true;
        Config_Update();
    }

    RESUME_INFO *const resume = Savegame_GetCurrentInfo(current_level);
    resume->flags.available = false;
    const bool bonus_level_unlock = Stats_CheckAllSecretsCollected(GFL_NORMAL);

    if (next_level != nullptr) {
        Savegame_PersistGameToCurrentInfo(next_level);
    }
    if (next_level == nullptr) {
        return (GF_COMMAND) { .action = GF_NOOP };
    }
    if (next_level->type == GFL_BONUS && !bonus_level_unlock) {
        return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
    }
    return (GF_COMMAND) {
        .action = GF_START_GAME,
        .param = next_level->num,
    };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleEnableSunset)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_STORY) {
        Output_SetSunsetEnabled(true);
    }
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandleDisableFloor)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_STORY) {
        Room_SetAbyssHeight((int16_t)(intptr_t)event->data);
    }
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandleSetStartAnim)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_STORY) {
        Lara_SetStartAnimState((LARA_EXTRA_STATE)(intptr_t)event->data);
    }
    return gf_cmd;
}

void GF_PreSequenceHook(
    const GF_SEQUENCE_CONTEXT seq_ctx, void *const seq_ctx_arg)
{
    Room_SetAbyssHeight(0);
    Output_SetSunsetEnabled(false);
    Lara_SetControllable(false);
    Lara_SetStartAnimState(LS_EXTRA_BREATH);
    if (seq_ctx == GFSC_SAVED) {
        Game_SetBonusFlag(GBF_NONE);
    }
}

GF_SEQUENCE_CONTEXT GF_SwitchSequenceContext(
    const GF_SEQUENCE_EVENT *const event, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    // Update sequence context if necessary
    if (event->type != GFS_LOOP_GAME) {
        return seq_ctx;
    }
    switch (seq_ctx) {
    case GFSC_SAVED:
    case GFSC_RESTART:
    case GFSC_SELECT:
        return GFSC_NORMAL;
    default:
        return seq_ctx;
    }
}

void GF_InitSequencer(void)
{
    for (GF_SEQUENCE_EVENT_TYPE event_type = 0; event_type < GFS_NUMBER_OF;
         event_type++) {
        if (m_EventHandlers[event_type] != nullptr) {
            GF_SetSequenceEventHandler(event_type, m_EventHandlers[event_type]);
        }
    }
}
