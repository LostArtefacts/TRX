#include "config.h"
#include "debug.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/game_flow/sequencer.h"
#include "game/game_flow/sequencer_priv.h"
#include "game/game_flow/vars.h"
#include "game/lara.h"
#include "game/lua.h"
#include "game/music.h"
#include "game/objects/creatures/bacon_lara.h"
#include "game/option/passport.h"
#include "game/output.h"
#include "game/phase.h"
#include "game/savegame.h"
#include "game/stats.h"
#include "log.h"
#include "version.h"

static DECLARE_GF_EVENT_HANDLER(M_HandleExitToTitle);
static DECLARE_GF_EVENT_HANDLER(M_HandleLevelComplete);
static DECLARE_GF_EVENT_HANDLER(M_HandlePlayLevel);
static DECLARE_GF_EVENT_HANDLER(M_HandlePlayCutscene);
static DECLARE_GF_EVENT_HANDLER(M_HandlePlayFMV);
static DECLARE_GF_EVENT_HANDLER(M_HandlePlayMusic);
static DECLARE_GF_EVENT_HANDLER(M_HandleInventoryModifier);
static DECLARE_GF_EVENT_HANDLER(M_HandlePicture);
static DECLARE_GF_EVENT_HANDLER(M_HandleLevelStats);
static DECLARE_GF_EVENT_HANDLER(M_HandleTotalStats);
static DECLARE_GF_EVENT_HANDLER(M_HandleEnableSunset);
static DECLARE_GF_EVENT_HANDLER(M_HandleSetupBaconLara);
static DECLARE_GF_EVENT_HANDLER(M_HandleDisableFloor);

static DECLARE_GF_EVENT_HANDLER((*m_EventHandlers[GFS_NUMBER_OF])) = {
    // clang-format off
    [GFS_EXIT_TO_TITLE]     = M_HandleExitToTitle,
    [GFS_LEVEL_COMPLETE]    = M_HandleLevelComplete,
    [GFS_LOOP_GAME]         = M_HandlePlayLevel,
    [GFS_PLAY_CUTSCENE]     = M_HandlePlayCutscene,
    [GFS_PLAY_FMV]          = M_HandlePlayFMV,
    [GFS_PLAY_MUSIC]        = M_HandlePlayMusic,
    [GFS_ADD_ITEM]          = M_HandleInventoryModifier,
    [GFS_REMOVE_WEAPONS]    = M_HandleInventoryModifier,
    [GFS_REMOVE_AMMO]       = M_HandleInventoryModifier,
    [GFS_REMOVE_MEDIPACKS]  = M_HandleInventoryModifier,
    [GFS_REMOVE_SCIONS]     = M_HandleInventoryModifier,
#if TR_VERSION > 1
    [GFS_ADD_SECRET_REWARD] = M_HandleInventoryModifier,
#endif
    [GFS_REMOVE_FLARES]     = M_HandleInventoryModifier,
    [GFS_LOADING_SCREEN]    = M_HandlePicture,
    [GFS_DISPLAY_PICTURE]   = M_HandlePicture,
    [GFS_LEVEL_STATS]       = M_HandleLevelStats,
    [GFS_TOTAL_STATS]       = M_HandleTotalStats,
    [GFS_ENABLE_SUNSET]     = M_HandleEnableSunset,
    [GFS_SETUP_BACON_LARA]  = M_HandleSetupBaconLara,
    [GFS_DISABLE_FLOOR]     = M_HandleDisableFloor,
    // clang-format on
};

static DECLARE_GF_EVENT_HANDLER(M_HandleExitToTitle)
{
    return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
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
    resume->flags.available = true;
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

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayLevel)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };

    if (seq_ctx == GFSC_STORY) {
        const int32_t savegame_level_num = (int32_t)(intptr_t)seq_ctx_arg;
        if (savegame_level_num == level->num) {
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        } else {
            return (GF_COMMAND) { .action = GF_NOOP };
        }
    }

    if (Lara_GetItem() != nullptr) {
        Lara_Initialise(level);
    }

    if (level->music_track != MX_INACTIVE) {
        Music_Stop();
    }

    Lua_FireEvent(LUA_EVENT_LEVEL_LOAD, level->num);

    // post load
    switch (seq_ctx) {
    case GFSC_SAVED: {
        const int16_t slot_num = Savegame_GetBoundSlot();
        if (!Savegame_Load(slot_num)) {
            LOG_ERROR("Failed to load save file!");
            Game_SetCurrentLevel(nullptr);
            GF_SetCurrentLevel(nullptr);
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }
        break;
    }

    default:
        if (level->type == GFL_NORMAL || level->type == GFL_BONUS) {
            Savegame_SetInitialVersion(SAVEGAME_CURRENT_VERSION);
            GF_InventoryModifier_Scan(Game_GetCurrentLevel());
            GF_InventoryModifier_Apply(Game_GetCurrentLevel(), GF_INV_REGULAR);
        }
        break;
    }

    if (level->type == GFL_NORMAL || level->type == GFL_BONUS) {
        Stats_CalculateStats();
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        if (resume != nullptr) {
#if TR_VERSION == 1
            resume->stats.max_pickup_count = Stats_GetMaxPickups();
            resume->stats.max_kill_count = Stats_GetMaxKillables();
#endif
            resume->stats.max_secret_count = Stats_GetMaxSecrets();
            resume->stats.all_secrets_mask = Stats_GetMaxSecretFlags();
        }
    }

    Lua_FireEvent(LUA_EVENT_LEVEL_START, level->num);

    g_Passport.ask_for_save = g_Config.gameplay.enable_save_crystals
        && seq_ctx == GFSC_NORMAL
        && GF_GetLevelTableType(level->type) == GFLT_MAIN
        && level != GF_GetFirstLevel() && level != GF_GetGymLevel();

    ASSERT(GF_GetCurrentLevel() == level);
    if (level->type == GFL_DEMO) {
        gf_cmd = GF_RunDemo(level->num);
    } else if (level->type == GFL_CUTSCENE) {
        gf_cmd = GF_RunCutscene(level->num);
    } else {
        if (seq_ctx != GFSC_SAVED && level != GF_GetFirstLevel()) {
            Lara_RevertToPistolsIfNeeded();
        }
        gf_cmd = GF_RunGame(level, seq_ctx);
    }
    if (gf_cmd.action == GF_LEVEL_COMPLETE) {
        gf_cmd.action = GF_NOOP;
    }
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayCutscene)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    const int16_t cutscene_num = (int16_t)(intptr_t)event->data;
    if (seq_ctx != GFSC_SAVED && g_Config.gameplay.enable_cutscenes) {
        gf_cmd = GF_DoCutsceneSequence(cutscene_num);
        if (gf_cmd.action == GF_LEVEL_COMPLETE) {
            gf_cmd.action = GF_NOOP;
        }
    }
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayFMV)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    const int16_t fmv_id = (int16_t)(intptr_t)event->data;
    if (seq_ctx == GFSC_SAVED) {
        return gf_cmd;
    }
    if (fmv_id < 0 || fmv_id >= g_GameFlow.fmv_count) {
        LOG_ERROR("Invalid FMV number: %d", fmv_id);
        return gf_cmd;
    }
    const GF_FMV *const fmv = &g_GameFlow.fmvs[fmv_id];
    if (fmv->is_legal && !g_Config.gameplay.enable_legal) {
        return gf_cmd;
    }
    if (fmv->is_credit && !g_Config.gameplay.enable_credits) {
        return gf_cmd;
    }
    FMV_Play(fmv->path);
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayMusic)
{
    if (seq_ctx != GFSC_STORY) {
        Music_Play_Direct((int32_t)(intptr_t)event->data, MPM_ALWAYS);
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandlePicture)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (event->type == GFS_LOADING_SCREEN) {
        if (!g_Config.gameplay.enable_loading_screens) {
            return gf_cmd;
        }
        if (seq_ctx == GFSC_STORY) {
            return gf_cmd;
        }
        Music_Stop();
    }
    if (seq_ctx == GFSC_SAVED) {
        return gf_cmd;
    }

    GF_DISPLAY_PICTURE_DATA *data = event->data;
    if (data->is_legal && !g_Config.gameplay.enable_legal) {
        return gf_cmd;
    }
    if (data->is_credit && !g_Config.gameplay.enable_credits) {
        return gf_cmd;
    }

    PHASE *const phase = Phase_Picture_Create((PHASE_PICTURE_ARGS) {
        .file_name = data->path,
        .display_time = data->display_time,
        .fade_in_time = data->fade_in_time,
        .fade_out_time = data->fade_out_time,
        .display_time_includes_fades = g_TRVersion >= 2,
    });
    gf_cmd = PhaseExecutor_Run(phase);
    Phase_Picture_Destroy(phase);
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandleInventoryModifier)
{
    // handled in GF_InventoryModifier_Apply
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleLevelStats)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_NORMAL) {
        return gf_cmd;
    }

#if TR_VERSION == 1
    const bool use_bare_style = g_Config.ui.stat_detail_mode != SDM_FULL;
#else
    const bool use_bare_style = false;
#endif

    PHASE *const phase = Phase_Stats_Create((PHASE_STATS_ARGS) {
        .background_type = (g_TRVersion == 1 || Game_IsInGym())
            ? BK_TRANSPARENT
            : g_Config.ui.stats_background_style,
        .level_num = -1,
        .show_final_stats = false,
        .use_bare_style = use_bare_style,
    });
    gf_cmd = PhaseExecutor_Run(phase);
    Phase_Stats_Destroy(phase);
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandleTotalStats)
{
    GF_COMMAND gf_cmd = { .action = GF_EXIT_TO_TITLE };
    if (seq_ctx != GFSC_NORMAL) {
        return gf_cmd;
    }
#if TR_VERSION == 1
    if (!g_Config.gameplay.enable_total_stats) {
        return gf_cmd;
    }
#endif
    PHASE *const phase = Phase_Stats_Create((PHASE_STATS_ARGS) {
        .background_type = BK_IMAGE,
        .background_path = event->data,
        .show_final_stats = true,
        .use_bare_style = false,
        .level_num = -1,
    });
    gf_cmd = PhaseExecutor_Run(phase);
    Phase_Stats_Destroy(phase);
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandleEnableSunset)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_STORY) {
        Output_SetSunsetEnabled(true);
    }
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandleSetupBaconLara)
{
    // TODO: move me to lua!
    if (seq_ctx != GFSC_STORY) {
        const int32_t anchor_room = (int32_t)(intptr_t)event->data;
        if (!BaconLara_InitialiseAnchor(anchor_room)) {
            LOG_ERROR("Could not anchor Bacon Lara to room %d", anchor_room);
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleDisableFloor)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_STORY) {
        Room_SetAbyssHeight((int16_t)(intptr_t)event->data);
    }
    return gf_cmd;
}

void GF_SetSequenceEventHandler(
    const GF_SEQUENCE_EVENT_TYPE event_type,
    const GF_SEQUENCE_EVENT_HANDLER event_handler)
{
    m_EventHandlers[event_type] = event_handler;
}

GF_SEQUENCE_EVENT_HANDLER GF_GetSequenceEventHandler(
    const GF_SEQUENCE_EVENT_TYPE event_type)
{
    return m_EventHandlers[event_type];
}
