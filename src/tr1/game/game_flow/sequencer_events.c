#include "game/game.h"
#include "game/lara.h"
#include "game/savegame.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/lua.h>
#include <libtrx/game/music.h>

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayLevel);
static DECLARE_GF_EVENT_HANDLER(M_HandlePlayMusic);
static DECLARE_GF_EVENT_HANDLER(M_HandleLevelComplete);
static DECLARE_GF_EVENT_HANDLER(M_HandleSetCameraPos);
static DECLARE_GF_EVENT_HANDLER(M_HandleSetCameraAngle);
static DECLARE_GF_EVENT_HANDLER(M_HandleDisableFloor);
static DECLARE_GF_EVENT_HANDLER(M_HandleFlipMap);
static DECLARE_GF_EVENT_HANDLER(M_HandleMeshSwap);

static DECLARE_GF_EVENT_HANDLER((*m_EventHandlers[GFS_NUMBER_OF])) = {
    // clang-format off
    [GFS_LOOP_GAME]        = M_HandlePlayLevel,
    [GFS_PLAY_MUSIC]       = M_HandlePlayMusic,
    [GFS_LEVEL_COMPLETE]   = M_HandleLevelComplete,
    [GFS_SET_CAMERA_POS]   = M_HandleSetCameraPos,
    [GFS_SET_CAMERA_ANGLE] = M_HandleSetCameraAngle,
    [GFS_DISABLE_FLOOR]    = M_HandleDisableFloor,
    [GFS_FLIP_MAP]         = M_HandleFlipMap,
    [GFS_MESH_SWAP]        = M_HandleMeshSwap,
    // clang-format on
};

static DECLARE_GF_EVENT_HANDLER(M_HandlePlayLevel)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    const GF_LEVEL *const prev_level = GF_GetLevelBefore(level);

    if (seq_ctx == GFSC_STORY) {
        const int32_t savegame_level_num = (int32_t)(intptr_t)seq_ctx_arg;
        if (savegame_level_num == level->num) {
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        } else {
            return (GF_COMMAND) { .action = GF_NOOP };
        }
    }

    // clear the save slot information so that /play starts with a fresh state
    g_GameInfo.select_save_slot = -1;

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
            GF_InventoryModifier_Scan(Game_GetCurrentLevel());
            GF_InventoryModifier_Apply(Game_GetCurrentLevel(), GF_INV_REGULAR);
        }
        break;
    }

    if (level->type == GFL_NORMAL || level->type == GFL_BONUS) {
        Stats_CalculateStats();
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        if (resume != nullptr) {
            resume->stats.max_pickup_count = Stats_GetMaxPickups();
            resume->stats.max_kill_count = Stats_GetMaxKillables();
            resume->stats.max_secret_count = Stats_GetMaxSecrets();
            resume->stats.all_secrets_mask = Stats_GetMaxSecretFlags();
        }
    }

    Lua_FireEvent(LUA_EVENT_LEVEL_START, level->num);

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
    Music_Play_Direct((int32_t)(intptr_t)event->data, MPM_ALWAYS);
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
    const bool bonus_level_unlock = Stats_CheckAllSecretsCollected(GFL_NORMAL);

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

static DECLARE_GF_EVENT_HANDLER(M_HandleDisableFloor)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    if (seq_ctx != GFSC_STORY) {
        Room_SetAbyssHeight((int16_t)(intptr_t)event->data);
    }
    return gf_cmd;
}

static DECLARE_GF_EVENT_HANDLER(M_HandleFlipMap)
{
    if (seq_ctx != GFSC_STORY) {
        Room_FlipMap();
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

void GF_PreSequenceHook(
    const GF_SEQUENCE_CONTEXT seq_ctx, void *const seq_ctx_arg)
{
    Room_SetAbyssHeight(0);
    Lara_SetControllable(false);
    if (seq_ctx == GFSC_SAVED) {
        Game_SetBonusFlag(GBF_NONE);
    }
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

void GF_InitSequencer(void)
{
    for (GF_SEQUENCE_EVENT_TYPE event_type = 0; event_type < GFS_NUMBER_OF;
         event_type++) {
        if (m_EventHandlers[event_type] != nullptr) {
            GF_SetSequenceEventHandler(event_type, m_EventHandlers[event_type]);
        }
    }
}
