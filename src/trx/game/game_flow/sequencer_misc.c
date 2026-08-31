#include <trx/config.h>
#include <trx/core/enum_map.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/debug.h>
#include <trx/game/demo.h>
#include <trx/game/fmv.h>
#include <trx/game/game.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/game_flow/sequencer.h>
#include <trx/game/game_flow/vars.h>
#include <trx/game/game_strings/table.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring/control.h>
#include <trx/game/level.h>
#include <trx/game/objects/links.h>
#include <trx/game/phase.h>
#include <trx/game/savegame.h>
#include <trx/game/shell/common.h>

static void M_PlayIntroFMVs(void)
{
    if (g_Config.gameplay.intro_fmv_mode != INTRO_FMV_LAUNCH) {
        return;
    }

    for (int32_t i = 0; i < g_GameFlow.fmv_count; i++) {
        const GF_FMV *const fmv = &g_GameFlow.fmvs[i];
        if (fmv->is_intro) {
            SHOULD(FMV_Play(fmv->path));
        }
    }
}

RESULT GF_RunTitle(GF_COMMAND *const out_cmd)
{
    SG_Manager_UnbindSlot();
    GameStringTable_Apply(nullptr);
    const GF_LEVEL *const title_level = GF_GetTitleLevel();
    MUST(Level_Initialise(title_level, GFSC_NORMAL), "the title level");
    *out_cmd = GF_ShowInventory(INV_TITLE_MODE);
    return OK;
}

GF_COMMAND GF_EnterPhotoMode(void)
{
    PHASE *const subphase = Phase_PhotoMode_Create();
    const GF_COMMAND gf_cmd = PhaseExecutor_Run(subphase);
    Phase_PhotoMode_Destroy(subphase);
    return gf_cmd;
}

GF_COMMAND GF_PauseGame(void)
{
    PHASE *const subphase = Phase_Pause_Create();
    const GF_COMMAND gf_cmd = PhaseExecutor_Run(subphase);
    Phase_Pause_Destroy(subphase);
    return gf_cmd;
}

GF_COMMAND GF_ShowInventory(const INVENTORY_MODE mode)
{
    if (Phase_SaveLoad_IsAvailable(mode)) {
        PHASE *const phase = Phase_SaveLoad_Create(mode);
        const GF_COMMAND gf_cmd = PhaseExecutor_Run(phase);
        Phase_SaveLoad_Destroy(phase);
        return gf_cmd;
    }

    PHASE *const phase = Phase_Inventory_Create(mode);
    const GF_COMMAND gf_cmd = PhaseExecutor_Run(phase);
    Phase_Inventory_Destroy(phase);
    return gf_cmd;
}

bool GF_ShowInventoryKeys(const OBJECT_ID receptacle_type_id)
{
    if (!InvRing_IsRingAvailable(RT_KEYS)) {
        return false;
    }
    if (g_Config.gameplay.enable_auto_item_selection) {
        const OBJECT_ID obj_id = ObjectLink_GetInverse(
            receptacle_type_id, OBJ_LINK_KEY_TO_RECEPTACLE);
        InvRing_SetRequestedObjectID(obj_id);
    } else {
        InvRing_ClearSelection();
    }
    const GF_COMMAND gf_cmd = GF_ShowInventory(INV_KEYS_MODE);
    if (gf_cmd.action != GF_NOOP) {
        GF_OverrideCommand(gf_cmd);
    }
    return true;
}

GF_COMMAND GF_RunDemo(const int32_t demo_num)
{
    PHASE *const demo_phase = Phase_Demo_Create(demo_num);
    const GF_COMMAND gf_cmd = PhaseExecutor_Run(demo_phase);
    Phase_Demo_Destroy(demo_phase);
    return gf_cmd;
}

GF_COMMAND GF_RunCutscene(const int32_t cutscene_num, const bool cross_fade_in)
{
    PHASE *const cutscene_phase = Phase_Cutscene_Create((PHASE_CUTSCENE_ARGS) {
        .level_num = cutscene_num,
        .cross_fade_in = cross_fade_in,
    });
    const GF_COMMAND gf_cmd = PhaseExecutor_Run(cutscene_phase);
    Phase_Cutscene_Destroy(cutscene_phase);
    return gf_cmd;
}

GF_COMMAND GF_RunGame(
    const GF_LEVEL *const level, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    PHASE *const phase = Phase_Game_Create(level, seq_ctx);
    const GF_COMMAND gf_cmd = PhaseExecutor_Run(phase);
    Phase_Game_Destroy(phase);
    return gf_cmd;
}

GF_COMMAND GF_RunGlobeSelect(const char *const background_path)
{
    PHASE *const phase = Phase_GlobeSelect_Create((PHASE_GLOBE_SELECT_ARGS) {
        .background_path = background_path,
    });
    const GF_COMMAND gf_cmd = PhaseExecutor_Run(phase);
    Phase_GlobeSelect_Destroy(phase);
    return gf_cmd;
}

RESULT GF_DoFrontendSequence(GF_COMMAND *const out_cmd)
{
    const SHELL_ARGS *const args = Shell_GetArgs();
    if (args != nullptr) {
        if (args->startup.save_to_load >= 0) {
            *out_cmd = (GF_COMMAND) {
                .action = GF_START_SAVED_GAME,
                .param = SG_Manager_SlotToParam(
                    SG_Manager_NormalSlot(args->startup.save_to_load)),
            };
            return OK;
        }

        if (args->startup.level_request.num >= 0) {
            const GF_LEVEL *const level = GF_GetLevelByOrdinalNumber(
                GFLT_MAIN, args->startup.level_request.num);
            FAIL_IF(
                level == nullptr, "there is no level number %d",
                args->startup.level_request.num);
            *out_cmd = (GF_COMMAND) {
                .action = GF_SELECT_GAME,
                .param = level->num,
            };
            return OK;
        }

        if (args->startup.level_request.query != nullptr) {
            const GF_LEVEL *const level =
                GF_FindPlayableLevelByQuery(args->startup.level_request.query);
            FAIL_IF(
                level == nullptr,
                "there is no level file '%s', and no game flow level title "
                "matches it",
                args->startup.level_request.query);
            *out_cmd = (GF_COMMAND) {
                .action = GF_SELECT_GAME,
                .param = level->num,
            };
            return OK;
        }

        if (args->startup.level_request.path != nullptr) {
            *out_cmd = (GF_COMMAND) {
                .action = GF_START_GAME,
                .param = 0,
            };
            return OK;
        }
    }

    if (g_GameFlow.title_level == nullptr) {
        *out_cmd = (GF_COMMAND) { .action = GF_NOOP };
        return OK;
    }

    GF_COMMAND gf_cmd;
    MUST(GF_InterpretSequence(
        g_GameFlow.title_level, GFSC_NORMAL, nullptr, &gf_cmd));
    if (gf_cmd.action == GF_NOOP || gf_cmd.action == GF_EXIT_TO_TITLE) {
        M_PlayIntroFMVs();
    }
    *out_cmd = gf_cmd;
    return OK;
}

RESULT GF_DoLevelSequence(
    const GF_LEVEL *const start_level, const GF_SEQUENCE_CONTEXT seq_ctx,
    GF_COMMAND *const out_cmd)
{
    const GF_LEVEL *current_level = start_level;
    const GF_LEVEL_TABLE_TYPE level_table_type =
        GF_GetLevelTableType(current_level->type);
    const int32_t level_count = GF_GetLevelTable(level_table_type)->count;
    while (true) {
        GF_COMMAND gf_cmd;
        MUST(GF_InterpretSequence(current_level, seq_ctx, nullptr, &gf_cmd));

        if (gf_cmd.action != GF_NOOP && gf_cmd.action != GF_LEVEL_COMPLETE) {
            *out_cmd = gf_cmd;
            return OK;
        }
        if (Game_IsInGym() || current_level->num + 1 >= level_count) {
            *out_cmd = (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
            return OK;
        }
        current_level++;
    }
}

RESULT GF_DoDemoSequence(int32_t demo_num, GF_COMMAND *const out_cmd)
{
    demo_num = Demo_ChooseLevel(demo_num);
    if (demo_num < 0) {
        // There is nothing to demonstrate, which is not a fault.
        *out_cmd = (GF_COMMAND) { .action = GF_NOOP };
        return OK;
    }
    const GF_LEVEL *const level = GF_GetLevel(GFLT_DEMOS, demo_num);
    FAIL_IF(level == nullptr, "the game flow has no demo %d", demo_num);
    return GF_InterpretSequence(level, GFSC_NORMAL, nullptr, out_cmd);
}

RESULT GF_DoCutsceneSequence(
    const int32_t cutscene_num, const bool cross_fade_in,
    GF_COMMAND *const out_cmd)
{
    const GF_LEVEL *const level = GF_GetLevel(GFLT_CUTSCENES, cutscene_num);
    FAIL_IF(level == nullptr, "the game flow has no cutscene %d", cutscene_num);
    return GF_InterpretSequence(
        level, GFSC_NORMAL, (void *)(intptr_t)cross_fade_in, out_cmd);
}

RESULT GF_PlayAvailableStory(
    const SAVEGAME_SLOT_REF slot, GF_COMMAND *const out_cmd)
{
    const int32_t savegame_level = SG_Manager_GetLevelNumber(slot);
    const bool prev_enable_legal = g_Config.gameplay.enable_legal;
    CONFIG_SET(g_Config.gameplay.enable_legal, false);

    // Play intro FMVs and cutscenes
    GF_COMMAND intro_cmd;
    SHOULD(
        GF_DoFrontendSequence(&intro_cmd),
        "The story plays from the save rather than from the start");

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i <= MIN(savegame_level, level_table->count); i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        if (level->type == GFL_GYM) {
            continue;
        }
        GF_COMMAND gf_cmd;
        MUST(GF_InterpretSequence(
            level, GFSC_STORY, (void *)(intptr_t)savegame_level, &gf_cmd));
        if (gf_cmd.action == GF_EXIT_TO_TITLE
            || gf_cmd.action == GF_EXIT_GAME) {
            break;
        }
    }

    CONFIG_SET(g_Config.gameplay.enable_legal, prev_enable_legal);
    *out_cmd = (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
    return OK;
}

bool GF_HasAvailableStory(const SAVEGAME_SLOT_REF slot)
{
    if (SG_Manager_IsSlotFree(slot)) {
        return false;
    }
    const int32_t savegame_level = SG_Manager_GetLevelNumber(slot);

    // Check intro FMVs and cutscenes in frontend sequence (skip legal FMVs)
    const GF_LEVEL *const title_level = GF_GetTitleLevel();
    if (title_level != nullptr) {
        const GF_SEQUENCE *const seq = &title_level->sequence;
        for (int32_t j = 0; j < seq->length; j++) {
            const GF_SEQUENCE_EVENT *const ev = &seq->events[j];
            if (ev->type == GFS_PLAY_CUTSCENE) {
                return true;
            }
            if (ev->type == GFS_PLAY_FMV) {
                const int32_t fmv_id = (int32_t)(intptr_t)ev->data;
                if (fmv_id < 0 || fmv_id >= g_GameFlow.fmv_count) {
                    continue;
                }
                const GF_FMV *const fmv = &g_GameFlow.fmvs[fmv_id];
                if (!fmv->is_legal && !fmv->is_credit) {
                    return true;
                }
            }
        }
    }

    // Check for any cutscenes or FMVs up until the save point
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    const int32_t max_level = MIN(savegame_level, level_table->count);
    for (int32_t i = 0; i <= max_level; i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        if (level->type == GFL_GYM) {
            continue;
        }
        const GF_SEQUENCE *const seq = &level->sequence;
        for (int32_t j = 0; j < seq->length; j++) {
            const GF_SEQUENCE_EVENT *const ev = &seq->events[j];
            // Stop checking after the saved level
            if (ev->type == GFS_LOOP_GAME) {
                break;
            }
            if (ev->type == GFS_PLAY_CUTSCENE) {
                return true;
            }
            if (ev->type == GFS_PLAY_FMV) {
                const int32_t fmv_id = (int32_t)(intptr_t)ev->data;
                if (fmv_id < 0 || fmv_id >= g_GameFlow.fmv_count) {
                    continue;
                }
                const GF_FMV *const fmv = &g_GameFlow.fmvs[fmv_id];
                if (!fmv->is_legal && !fmv->is_credit) {
                    return true;
                }
            }
        }
    }
    return false;
}

RESULT GF_RunUntilExit(GF_COMMAND gf_cmd)
{
    bool loop_continue = !Shell_IsExiting();
    while (loop_continue) {
        LOG_INFO(
            "action=%s param=%d", ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action),
            gf_cmd.param);

        switch (gf_cmd.action) {
        case GF_START_GAME:
        case GF_SELECT_GAME: {
            const int32_t level_num = gf_cmd.param;
            const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, level_num);
            const GF_SEQUENCE_CONTEXT seq_ctx =
                gf_cmd.action == GF_SELECT_GAME ? GFSC_SELECT : GFSC_NORMAL;
            if (level != nullptr) {
                MUST(GF_DoLevelSequence(level, seq_ctx, &gf_cmd));
            }
            break;
        }

        case GF_GLOBE_SELECT:
            gf_cmd = GF_RunGlobeSelect(nullptr);
            break;

        case GF_START_SAVED_GAME: {
            const SAVEGAME_SLOT_REF slot =
                SG_Manager_SlotFromParam(gf_cmd.param);
            const int32_t level_num = SG_Manager_GetLevelNumber(slot);
            if (level_num < 0) {
                LOG_ERROR("Corrupt save file!");
                gf_cmd = (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
            } else {
                SG_Manager_BindSlot(slot);
                const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, level_num);
                MUST(GF_DoLevelSequence(level, GFSC_SAVED, &gf_cmd));
            }
            break;
        }

        case GF_RESTART_GAME: {
            const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, gf_cmd.param);
            MUST(GF_InterpretSequence(level, GFSC_RESTART, nullptr, &gf_cmd));
            break;
        }

        case GF_STORY_SO_FAR:
            MUST(GF_PlayAvailableStory(
                SG_Manager_SlotFromParam(gf_cmd.param), &gf_cmd));
            break;

        case GF_START_CINE:
            MUST(GF_DoCutsceneSequence(gf_cmd.param, false, &gf_cmd));
            break;

        case GF_START_DEMO:
            MUST(GF_DoDemoSequence(gf_cmd.param, &gf_cmd));
            break;

        case GF_NOOP:
        case GF_LEVEL_COMPLETE:
            gf_cmd = (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
            break;

        case GF_EXIT_TO_TITLE:
            if (Shell_GetArgs()->startup.level_request.path != nullptr) {
                gf_cmd = (GF_COMMAND) { .action = GF_EXIT_GAME };
            } else if (g_GameFlow.title_level == nullptr) {
                return FAIL("the game flow names no title level");
            } else {
                MUST(GF_RunTitle(&gf_cmd));
            }
            break;

        case GF_EXIT_GAME:
        case GF_SWITCH_MOD:
            loop_continue = false;
            break;

        default:
            ASSERT_FAIL_FMT(
                "invalid action (action=%s, param=%d)",
                ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action), gf_cmd.param);
        }
    }

    if (GF_GetCurrentLevel() != nullptr) {
        Level_Unload();
    }
    Game_SetCurrentLevel(nullptr);
    GF_SetCurrentLevel(nullptr);
    return OK;
}
