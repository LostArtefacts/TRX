#include "game/game_flow/sequencer.h"

#include "debug.h"
#include "enum_map.h"
#include "game/game.h"
#include "game/game_flow/sequencer_priv.h"
#include "game/inventory.h"
#include "game/lara/common.h"
#include "game/level.h"
#include "game/lua.h"
#include "game/savegame.h"

static GF_COMMAND M_RunEvent(
    const GF_LEVEL *const level, const GF_SEQUENCE_EVENT *const event,
    const GF_SEQUENCE_CONTEXT seq_ctx, void *const seq_ctx_arg)
{
    GF_COMMAND gf_cmd = { .action = GF_NOOP };
    LOG_DEBUG(
        "event type=%s(%d) data=0x%x",
        ENUM_MAP_TO_STRING(GF_SEQUENCE_EVENT_TYPE, event->type), event->type,
        event->data);

    const GF_SEQUENCE_EVENT_HANDLER event_handler =
        GF_GetSequenceEventHandler(event->type);
    if (event_handler == nullptr) {
        return gf_cmd;
    }

    gf_cmd = event_handler(level, event, seq_ctx, seq_ctx_arg);
    LOG_DEBUG(
        "event type=%s(%d) data=0x%x finished, result: action=%s, "
        "param=%d",
        ENUM_MAP_TO_STRING(GF_SEQUENCE_EVENT_TYPE, event->type), event->type,
        event->data, ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action),
        gf_cmd.param);
    return gf_cmd;
}

GF_COMMAND GF_InterpretSequence(
    const GF_LEVEL *const level, GF_SEQUENCE_CONTEXT seq_ctx,
    void *const seq_ctx_arg)
{
    ASSERT(level != nullptr);
    LOG_DEBUG(
        "running sequence for level=%d type=%d seq_ctx=%d", level->num,
        level->type, seq_ctx);

    if (level->type == GFL_DUMMY || level->type == GFL_CURRENT) {
        return (GF_COMMAND) { .action = GF_NOOP };
    }

    GF_PreSequenceHook(seq_ctx, seq_ctx_arg);

    GF_COMMAND gf_cmd = { .action = GF_EXIT_TO_TITLE };

    const GF_LEVEL *const prev_level = GF_GetLevelBefore(level);

    // before load
    switch (seq_ctx) {
    case GFSC_STORY:
        break;

    case GFSC_SAVED:
        GF_InventoryModifier_Scan(level);
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
        if (level->type == GFL_NORMAL || level->type == GFL_BONUS) {
            GF_InventoryModifier_Scan(level);
            GF_InventoryModifier_ApplyToResumeInfo(level);
        }
        break;

    case GFSC_SELECT: {
        const int16_t slot_num = Savegame_GetBoundSlot();
        if (slot_num != -1) {
            // select level feature
            Savegame_InitCurrentInfo();
            if (level->num > GF_GetFirstLevel()->num) {
                Savegame_LoadOnlyResumeInfo(slot_num);
                const GF_LEVEL *tmp_level = level;
                while (tmp_level != nullptr) {
                    Savegame_ResetCurrentInfo(tmp_level);
                    tmp_level = GF_GetLevelAfter(tmp_level);
                }
                Savegame_CarryCurrentInfoToNextLevel(prev_level, level);
                Savegame_ApplyLogicToCurrentInfo(level);
                GF_InventoryModifier_Scan(level);
                GF_InventoryModifier_ApplyToResumeInfo(level);
            }
        } else {
            // console /play level feature
            Inv_RemoveAllItems();
            if (level == GF_GetGymLevel()) {
                Savegame_InitCurrentInfo();
                GF_InventoryModifier_Scan(level);
                GF_InventoryModifier_ApplyToResumeInfo(level);
            } else {
                const GF_LEVEL *tmp_level = GF_GetFirstLevel();
                Savegame_ResetCurrentInfo(tmp_level);
                while (tmp_level != nullptr && tmp_level <= level) {
                    Savegame_ApplyLogicToCurrentInfo(tmp_level);
                    GF_InventoryModifier_Scan(tmp_level);
                    GF_InventoryModifier_ApplyToResumeInfo(tmp_level);
                    if (tmp_level == level) {
                        break;
                    }

                    const GF_LEVEL *const next_level =
                        GF_GetLevelAfter(tmp_level);
                    if (next_level != nullptr) {
                        Savegame_CarryCurrentInfoToNextLevel(
                            tmp_level, next_level);
                    }
                    tmp_level = next_level;
                }
            }
        }
        break;
    }

    default:
        if (level->type == GFL_GYM) {
            Savegame_ResetCurrentInfo(level);
        } else if (
            prev_level != nullptr
            && (level->type == GFL_NORMAL || level->type == GFL_BONUS)) {
            Savegame_CarryCurrentInfoToNextLevel(prev_level, level);
        }
        Savegame_ApplyLogicToCurrentInfo(level);
        if (level->type == GFL_NORMAL || level->type == GFL_BONUS) {
            GF_InventoryModifier_Scan(level);
            GF_InventoryModifier_ApplyToResumeInfo(level);
        }
    }

    // load the level
    if (seq_ctx != GFSC_STORY || level->type == GFL_CUTSCENE) {
        if (!Level_Initialise(level, seq_ctx)) {
            Game_SetCurrentLevel(nullptr);
            GF_SetCurrentLevel(nullptr);
            if (level->type == GFL_TITLE) {
                gf_cmd = (GF_COMMAND) { .action = GF_EXIT_GAME };
            } else {
                gf_cmd = (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
            }
        }
    }

    // Run any level Lua script
    Lua_ClearLevelListeners();
    Lua_SetScriptContext(LUA_CONTEXT_LEVEL);
    if (level->script_path != nullptr) {
        LUA_RESULT res = Lua_EvalFile(level->script_path);
        if (res.code != LUA_OK) {
            LOG_ERROR("Lua level script error: %s", res.message);
        }
        Lua_FreeResult(&res);
    }
    Lua_SetScriptContext(LUA_CONTEXT_GLOBAL);

    const GF_SEQUENCE *const sequence = &level->sequence;
    for (int32_t i = 0; i < sequence->length; i++) {
        const GF_SEQUENCE_EVENT *const event = &sequence->events[i];
        gf_cmd = M_RunEvent(level, event, seq_ctx, seq_ctx_arg);
        if (gf_cmd.action != GF_NOOP) {
            return gf_cmd;
        }

        // Update sequence context if necessary
        seq_ctx = GF_SwitchSequenceContext(event, seq_ctx);
    }

    LOG_DEBUG(
        "sequence finished: action=%s param=%d",
        ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action), gf_cmd.param);
    return gf_cmd;
}
