#include "game/camera.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/game_flow/common.h"
#include "game/game_flow/sequencer.h"
#include "game/game_flow/vars.h"
#include "game/inventory.h"
#include "game/lara/common.h"
#include "game/level.h"
#include "game/music.h"
#include "game/objects/creatures/bacon_lara.h"
#include "game/savegame.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/phase.h>

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayLevel);
static DECLARE_GF_EVENT_HANDLER(M_HandlePlayMusic);
static DECLARE_GF_EVENT_HANDLER(M_HandleLevelComplete);
static DECLARE_GF_EVENT_HANDLER(M_HandleSetCameraPos);
static DECLARE_GF_EVENT_HANDLER(M_HandleSetCameraAngle);
static DECLARE_GF_EVENT_HANDLER(M_HandleFlipMap);
static DECLARE_GF_EVENT_HANDLER(M_HandleAddItem);
static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveWeapons);
static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveScions);
static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveAmmo);
static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveMedipacks);
static DECLARE_GF_EVENT_HANDLER(M_HandleMeshSwap);
static DECLARE_GF_EVENT_HANDLER(M_HandleSetupBaconLara);

static DECLARE_GF_EVENT_HANDLER((*m_EventHandlers[GFS_NUMBER_OF])) = {
    // clang-format off
    [GFS_LOOP_GAME]       = M_HandlePlayLevel,
    [GFS_PLAY_MUSIC]       = M_HandlePlayMusic,
    [GFS_LEVEL_COMPLETE]   = M_HandleLevelComplete,
    [GFS_SET_CAMERA_POS]   = M_HandleSetCameraPos,
    [GFS_SET_CAMERA_ANGLE] = M_HandleSetCameraAngle,
    [GFS_FLIP_MAP]         = M_HandleFlipMap,
    [GFS_ADD_ITEM]         = M_HandleAddItem,
    [GFS_REMOVE_WEAPONS]   = M_HandleRemoveWeapons,
    [GFS_REMOVE_SCIONS]    = M_HandleRemoveScions,
    [GFS_REMOVE_AMMO]      = M_HandleRemoveAmmo,
    [GFS_REMOVE_MEDIPACKS] = M_HandleRemoveMedipacks,
    [GFS_MESH_SWAP]        = M_HandleMeshSwap,
    [GFS_SETUP_BACON_LARA] = M_HandleSetupBaconLara,
    // clang-format on
};

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayLevel)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    const GF_LEVEL *const prev_level = GF_GetLevelBefore(level);

    // before load
    switch (seq_ctx) {
    case GFSC_STORY:
        const int32_t savegame_level_num = (int32_t)(intptr_t)seq_ctx_arg;
        if (savegame_level_num == level->num) {
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }
        break;

    case GFSC_SAVED:
        // reset current info to the defaults so that we do not do
        // Item_GlobalReplace in the inventory initialization routines too early
        Savegame_InitCurrentInfo();
        break;

    case GFSC_RESTART:
        if (level == GF_GetGymLevel() || level == GF_GetFirstLevel()) {
            Savegame_InitCurrentInfo();
        } else {
            Savegame_ResetCurrentInfo(level);
            Savegame_CarryCurrentInfoToNextLevel(prev_level, level);
            Savegame_ApplyLogicToCurrentInfo(level);
        }
        break;

    case GFSC_SELECT:
        if (Savegame_GetBoundSlot() != -1) {
            // select level feature
            Savegame_InitCurrentInfo();
            if (level->num > GF_GetFirstLevel()->num) {
                Savegame_LoadOnlyResumeInfo(
                    Savegame_GetBoundSlot(), &g_GameInfo);
                const GF_LEVEL *tmp_level = level;
                while (tmp_level != nullptr) {
                    Savegame_ResetCurrentInfo(tmp_level);
                    tmp_level = GF_GetLevelAfter(tmp_level);
                }
                Savegame_CarryCurrentInfoToNextLevel(prev_level, level);
                Savegame_ApplyLogicToCurrentInfo(level);
            }
        } else {
            // console /play level feature
            Savegame_InitCurrentInfo();
            const GF_LEVEL *tmp_level = GF_GetLevelAfter(GF_GetFirstLevel());
            while (tmp_level != nullptr) {
                Savegame_CarryCurrentInfoToNextLevel(
                    GF_GetLevelBefore(tmp_level), tmp_level);
                Savegame_ApplyLogicToCurrentInfo(tmp_level);
                if (tmp_level == level) {
                    break;
                }
                tmp_level = GF_GetLevelAfter(tmp_level);
            }
        }
        break;

    default:
        if (level->type == GFL_GYM) {
            Savegame_ResetCurrentInfo(level);
            Savegame_ApplyLogicToCurrentInfo(level);
        } else if (level->type == GFL_BONUS) {
            Savegame_CarryCurrentInfoToNextLevel(prev_level, level);
            Savegame_ApplyLogicToCurrentInfo(level);
        }
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
        break;
    }

    Stats_CalculateStats();
    RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
    if (resume != nullptr) {
        resume->stats.max_pickup_count = Stats_GetPickups();
        resume->stats.max_kill_count = Stats_GetKillables();
        resume->stats.max_secret_count = Stats_GetSecrets();
    }

    g_GameInfo.ask_for_save = g_Config.gameplay.enable_save_crystals
        && seq_ctx == GFSC_NORMAL
        && GF_GetLevelTableType(level->type) == GFLT_MAIN
        && level != GF_GetFirstLevel() && level != GF_GetGymLevel();

    if (gf_cmd.action != GF_NOOP) {
        return gf_cmd;
    }

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
        g_Config.profile.new_game_plus_unlock = true;
        Config_Write();
    }
    g_GameInfo.bonus_level_unlock = Stats_CheckAllSecretsCollected(GFL_NORMAL);

    // play specific level
    if (g_GameInfo.select_level_num != -1) {
        const GF_LEVEL *const select_level =
            GF_GetLevel(GFLT_MAIN, g_GameInfo.select_level_num);
        if (current_level != nullptr && select_level != nullptr) {
            Savegame_CarryCurrentInfoToNextLevel(current_level, select_level);
        }
        return (GF_COMMAND) {
            .action = GF_SELECT_GAME,
            .param = g_GameInfo.select_level_num,
        };
    }

    if (next_level == nullptr) {
        return (GF_COMMAND) { .action = GF_NOOP };
    }

    // carry info to the next level
    Savegame_CarryCurrentInfoToNextLevel(current_level, next_level);
    Savegame_ApplyLogicToCurrentInfo(next_level);

    if (next_level->type == GFL_BONUS && !g_GameInfo.bonus_level_unlock) {
        return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
    }
    return (GF_COMMAND) {
        .action = GF_START_GAME,
        .param = next_level->num,
    };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleSetCameraPos)
{
    if (seq_ctx != GFSC_STORY) {
        GF_SET_CAMERA_POS_DATA *const data = event->data;
        CINE_DATA *const cine_data = Camera_GetCineData();
        if (data->x.set) {
            cine_data->position.pos.x = (int32_t)(intptr_t)data->x.value;
        }
        if (data->y.set) {
            cine_data->position.pos.y = (int32_t)(intptr_t)data->y.value;
        }
        if (data->z.set) {
            cine_data->position.pos.z = (int32_t)(intptr_t)data->z.value;
        }
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleSetCameraAngle)
{
    if (seq_ctx != GFSC_STORY) {
        Camera_GetCineData()->position.rot.y = (int32_t)(intptr_t)event->data;
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleFlipMap)
{
    if (seq_ctx != GFSC_STORY) {
        Room_FlipMap();
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleAddItem)
{
    if (seq_ctx != GFSC_STORY && seq_ctx != GFSC_SAVED) {
        const GF_ADD_ITEM_DATA *add_item_data =
            (const GF_ADD_ITEM_DATA *)event->data;
        Inv_AddItemNTimes(add_item_data->object_id, add_item_data->quantity);
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveWeapons)
{
    if (seq_ctx != GFSC_STORY && seq_ctx != GFSC_SAVED
        && !(g_GameInfo.bonus_flag & GBF_NGPLUS)) {
        g_GameInfo.remove_guns = true;
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveAmmo)
{
    if (seq_ctx != GFSC_STORY && seq_ctx != GFSC_SAVED
        && !(g_GameInfo.bonus_flag & GBF_NGPLUS)) {
        g_GameInfo.remove_ammo = true;
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveScions)
{
    if (seq_ctx != GFSC_STORY && seq_ctx != GFSC_SAVED) {
        g_GameInfo.remove_scions = true;
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleRemoveMedipacks)
{
    if (seq_ctx != GFSC_STORY && seq_ctx != GFSC_SAVED) {
        g_GameInfo.remove_medipacks = true;
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleMeshSwap)
{
    if (seq_ctx != GFSC_STORY) {
        const GF_MESH_SWAP_DATA *const swap_data = event->data;
        Object_SwapMesh(
            swap_data->object1_id, swap_data->object2_id, swap_data->mesh_num);
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

static DECLARE_GF_EVENT_HANDLER(M_HandleSetupBaconLara)
{
    if (seq_ctx != GFSC_STORY) {
        const int32_t anchor_room = (int32_t)(intptr_t)event->data;
        if (!BaconLara_InitialiseAnchor(anchor_room)) {
            LOG_ERROR("Could not anchor Bacon Lara to room %d", anchor_room);
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }
    }
    return (GF_COMMAND) { .action = GF_NOOP };
}

void GF_PreSequenceHook(void)
{
    g_GameInfo.remove_guns = false;
    g_GameInfo.remove_scions = false;
    g_GameInfo.remove_ammo = false;
    g_GameInfo.remove_medipacks = false;
}

GF_SEQUENCE_CONTEXT GF_SwitchSequenceContext(
    const GF_SEQUENCE_EVENT *const event, const GF_SEQUENCE_CONTEXT seq_ctx)
{
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

bool GF_ShouldSkipSequenceEvent(
    const GF_LEVEL *const level, const GF_SEQUENCE_EVENT *const event)
{
    // Skip cinematic levels
    if (!g_Config.gameplay.enable_cine && level->type == GFL_CUTSCENE) {
        switch (event->type) {
        case GFS_EXIT_TO_TITLE:
        case GFS_LEVEL_COMPLETE:
        case GFS_PLAY_FMV:
        case GFS_LEVEL_STATS:
        case GFS_TOTAL_STATS:
            return false;
        default:
            return true;
        }
    }
    return false;
}

GF_EVENT_QUEUE_TYPE GF_ShouldDeferSequenceEvent(
    const GF_SEQUENCE_EVENT_TYPE event_type)
{
    switch (event_type) {
    case GFS_SET_CAMERA_POS:
    case GFS_SET_CAMERA_ANGLE:
    case GFS_FLIP_MAP:
    case GFS_ADD_ITEM:
    case GFS_MESH_SWAP:
    case GFS_SETUP_BACON_LARA:
        return GF_EVENT_QUEUE_AFTER_LEVEL_INIT;

    case GFS_REMOVE_WEAPONS:
    case GFS_REMOVE_SCIONS:
    case GFS_REMOVE_AMMO:
    case GFS_REMOVE_MEDIPACKS:
        return GF_EVENT_QUEUE_BEFORE_LEVEL_INIT;

    default:
        return GF_EVENT_QUEUE_NONE;
    };
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
