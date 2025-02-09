#include "decomp/decomp.h"
#include "decomp/savegame.h"
#include "game/camera.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_flow/sequencer.h"
#include "game/level.h"
#include "game/music.h"
#include "game/phase.h"
#include "global/vars.h"

#include <libtrx/debug.h>

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayLevel);
static DECLARE_GF_EVENT_HANDLER(M_HandlePlayMusic);
static DECLARE_GF_EVENT_HANDLER(M_HandleLevelComplete);
static DECLARE_GF_EVENT_HANDLER(M_HandleEnableSunset);
static DECLARE_GF_EVENT_HANDLER(M_HandleSetCameraAngle);
static DECLARE_GF_EVENT_HANDLER(M_HandleDisableFloor);
static DECLARE_GF_EVENT_HANDLER(M_HandleAddItem);
static DECLARE_GF_EVENT_HANDLER(M_HandleAddSecretReward);
static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveWeapons);
static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveAmmo);
static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveFlares);
static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveMedipacks);
static DECLARE_GF_EVENT_HANDLER(M_HandleSetStartAnim);
static DECLARE_GF_EVENT_HANDLER(M_HandleSetNumSecrets);

static DECLARE_GF_EVENT_HANDLER((*m_EventHandlers[GFS_NUMBER_OF])) = {
    // clang-format off
    [GFS_LOOP_GAME]        = M_HandlePlayLevel,
    [GFS_PLAY_MUSIC]        = M_HandlePlayMusic,
    [GFS_LEVEL_COMPLETE]    = M_HandleLevelComplete,
    [GFS_ENABLE_SUNSET]     = M_HandleEnableSunset,
    [GFS_SET_CAMERA_ANGLE]  = M_HandleSetCameraAngle,
    [GFS_DISABLE_FLOOR]     = M_HandleDisableFloor,
    [GFS_ADD_ITEM]          = M_HandleAddItem,
    [GFS_ADD_SECRET_REWARD] = M_HandleAddSecretReward,
    [GFS_REMOVE_WEAPONS]    = M_HandleRemoveWeapons,
    [GFS_REMOVE_AMMO]       = M_HandleRemoveAmmo,
    [GFS_REMOVE_FLARES]     = M_HandleRemoveFlares,
    [GFS_REMOVE_MEDIPACKS]  = M_HandleRemoveMedipacks,
    [GFS_SET_START_ANIM]    = M_HandleSetStartAnim,
    [GFS_SET_NUM_SECRETS]   = M_HandleSetNumSecrets,
    // clang-format on
};

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayLevel)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    switch (seq_ctx) {
    case GFSC_STORY:
        return gf_cmd;

    case GFSC_SAVED:
        break;

    case GFSC_SELECT: {
        // console /play level feature
        Savegame_InitCurrentInfo();
        const GF_LEVEL *tmp_level = GF_GetFirstLevel();
        while (tmp_level != nullptr && tmp_level <= level) {
            Savegame_ApplyLogicToCurrentInfo(tmp_level);
            GF_InventoryModifier_Scan(tmp_level);
            GF_InventoryModifier_Apply(tmp_level, GF_INV_REGULAR);
            if (tmp_level == level) {
                break;
            }
            const GF_LEVEL *const next_level = GF_GetLevelAfter(tmp_level);
            if (next_level != nullptr) {
                Savegame_CarryCurrentInfoToNextLevel(tmp_level, next_level);
            }
            tmp_level = next_level;
        }
        InitialiseLevelFlags();
        break;
    }

    default:
        Savegame_ApplyLogicToCurrentInfo(level);
        if (level->type == GFL_NORMAL) {
            GF_InventoryModifier_Scan(level);
            GF_InventoryModifier_Apply(level, GF_INV_REGULAR);
        }
        InitialiseLevelFlags();
        break;
    }

    gf_cmd = GF_RunSequencerQueue(
        GF_EVENT_QUEUE_BEFORE_LEVEL_INIT, level, seq_ctx, seq_ctx_arg);
    if (gf_cmd.action != GF_NOOP) {
        return gf_cmd;
    }

    // load the level
    if (!Level_Initialise(level)) {
        Game_SetCurrentLevel(nullptr);
        GF_SetCurrentLevel(nullptr);
        if (level->type == GFL_TITLE) {
            gf_cmd = (GF_COMMAND) { .action = GF_EXIT_GAME };
        } else {
            gf_cmd = (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }
    }

    gf_cmd = GF_RunSequencerQueue(
        GF_EVENT_QUEUE_AFTER_LEVEL_INIT, level, seq_ctx, seq_ctx_arg);
    if (gf_cmd.action != GF_NOOP) {
        return gf_cmd;
    }

    switch (seq_ctx) {
    case GFSC_SAVED:
        ExtractSaveGameInfo();
        break;

    default:
        if (level->type == GFL_NORMAL) {
            GF_InventoryModifier_Scan(Game_GetCurrentLevel());
            GF_InventoryModifier_Apply(Game_GetCurrentLevel(), GF_INV_REGULAR);
        }
        break;
    }

    ASSERT(GF_GetCurrentLevel() == level);
    if (level->type == GFL_DEMO) {
        gf_cmd = GF_RunDemo(level->num);
    } else if (level->type == GFL_CUTSCENE) {
        gf_cmd = GF_RunCutscene(level->num);
    } else {
        gf_cmd = GF_RunGame(level, seq_ctx);
    }
    if (gf_cmd.action == GF_LEVEL_COMPLETE) {
        gf_cmd.action = GF_NOOP;
    }
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayMusic)
{
    Music_Play((int32_t)(intptr_t)event->data, MPM_ALWAYS);
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
        g_SaveGame.bonus_flag = true;
        // TODO: refactor me
        START_INFO *const start = Savegame_GetCurrentInfo(current_level);
        start->stats = g_SaveGame.current_stats;
    }

    START_INFO *const start = Savegame_GetCurrentInfo(current_level);
    start->stats = g_SaveGame.current_stats;
    start->available = 0;
    if (next_level != nullptr) {
        Savegame_PersistGameToCurrentInfo(next_level);
        g_SaveGame.current_level = next_level->num;
    }
    if (next_level == nullptr) {
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
        g_GF_SunsetEnabled = true;
    }
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandleSetCameraAngle)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_SAVED) {
        Camera_GetCineData()->position.target_angle =
            (int16_t)(intptr_t)event->data;
    }
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandleDisableFloor)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_STORY) {
        g_GF_NoFloor = (int16_t)(intptr_t)event->data;
    }
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandleAddItem)
{
    // handled in GF_InventoryModifier_Apply
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleAddSecretReward)
{
    // handled in GF_InventoryModifier_Apply
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveWeapons)
{
    // handled in GF_InventoryModifier_Apply
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveAmmo)
{
    // handled in GF_InventoryModifier_Apply
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveFlares)
{
    // handled in GF_InventoryModifier_Apply
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveMedipacks)
{
    // handled in GF_InventoryModifier_Apply
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleSetStartAnim)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_STORY) {
        g_GF_LaraStartAnim = (int16_t)(intptr_t)event->data;
    }
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandleSetNumSecrets)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_STORY) {
        g_GF_NumSecrets = (int16_t)(intptr_t)event->data;
    }
    return gf_cmd;
}

void GF_PreSequenceHook(void)
{
    g_GF_NoFloor = 0;
    g_GF_SunsetEnabled = false;
    g_GF_LaraStartAnim = 0;
    g_GF_RemoveAmmo = false;
    g_GF_RemoveWeapons = false;
    g_GF_NumSecrets = 3;
    Camera_GetCineData()->position.target_angle = DEG_90;
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
    case GFSC_SELECT:
        return GFSC_NORMAL;
    default:
        return seq_ctx;
    }
}

bool GF_ShouldSkipSequenceEvent(
    const GF_LEVEL *const level, const GF_SEQUENCE_EVENT *const event)
{
    return false;
}

GF_EVENT_QUEUE_TYPE GF_ShouldDeferSequenceEvent(
    const GF_SEQUENCE_EVENT_TYPE event_type)
{
    return GF_EVENT_QUEUE_NONE;
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
