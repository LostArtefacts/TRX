#include "config.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/game_flow/sequencer.h"
#include "game/game_flow/sequencer_priv.h"
#include "game/game_flow/vars.h"
#include "game/objects/creatures/bacon_lara.h"
#include "game/phase.h"
#include "log.h"

static DECLARE_GF_EVENT_HANDLER(M_HandleExitToTitle);
static DECLARE_GF_EVENT_HANDLER(M_HandlePlayCutscene);
static DECLARE_GF_EVENT_HANDLER(M_HandlePlayFMV);
static DECLARE_GF_EVENT_HANDLER(M_HandlePicture);
static DECLARE_GF_EVENT_HANDLER(M_HandleInventoryModifier);
static DECLARE_GF_EVENT_HANDLER(M_HandleLevelStats);
static DECLARE_GF_EVENT_HANDLER(M_HandleTotalStats);
static DECLARE_GF_EVENT_HANDLER(M_HandleSetupBaconLara);

static DECLARE_GF_EVENT_HANDLER((*m_EventHandlers[GFS_NUMBER_OF])) = {
    // clang-format off
    [GFS_EXIT_TO_TITLE]     = M_HandleExitToTitle,
    [GFS_PLAY_CUTSCENE]     = M_HandlePlayCutscene,
    [GFS_PLAY_FMV]          = M_HandlePlayFMV,
    [GFS_ADD_ITEM]          = M_HandleInventoryModifier,
    [GFS_REMOVE_WEAPONS]    = M_HandleInventoryModifier,
    [GFS_REMOVE_AMMO]       = M_HandleInventoryModifier,
    [GFS_REMOVE_MEDIPACKS]  = M_HandleInventoryModifier,
#if TR_VERSION == 1
    [GFS_REMOVE_SCIONS]     = M_HandleInventoryModifier,
    [GFS_LOADING_SCREEN]    = M_HandlePicture,
#else
    [GFS_ADD_SECRET_REWARD] = M_HandleInventoryModifier,
    [GFS_REMOVE_FLARES]     = M_HandleInventoryModifier,
#endif
    [GFS_DISPLAY_PICTURE]   = M_HandlePicture,
    [GFS_LEVEL_STATS]       = M_HandleLevelStats,
    [GFS_TOTAL_STATS]       = M_HandleTotalStats,
    [GFS_SETUP_BACON_LARA]  = M_HandleSetupBaconLara,
    // clang-format on
};

static DECLARE_GF_EVENT_HANDLER(M_HandleExitToTitle)
{
    return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
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

static DECLARE_GF_EVENT_HANDLER(M_HandlePicture)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
#if TR_VERSION == 1
    if (event->type == GFS_LOADING_SCREEN
        && !g_Config.gameplay.enable_loading_screens) {
        return gf_cmd;
    }
#endif
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
        .display_time_includes_fades = TR_VERSION >= 2,
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
        .background_type = (TR_VERSION == 1 || Game_IsInGym())
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
