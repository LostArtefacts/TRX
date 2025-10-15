#include "game/demo.h"
#include "game/game.h"
#include "game/game_flow/common.h"
#include "game/game_flow/sequencer.h"
#include "game/game_flow/vars.h"
#include "game/savegame.h"

#include <libtrx/config.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/game/level.h>
#include <libtrx/log.h>
#include <libtrx/utils.h>

GF_COMMAND GF_RunTitle(void)
{
    Savegame_UnbindSlot();
    GameStringTable_Apply(nullptr);
    const GF_LEVEL *const title_level = GF_GetTitleLevel();
    if (!Level_Initialise(title_level, GFSC_NORMAL)) {
        return (GF_COMMAND) { .action = GF_EXIT_GAME };
    }
    return GF_ShowInventory(INV_TITLE_MODE);
}

GF_COMMAND GF_PlayAvailableStory(const int32_t slot_num)
{
    const int32_t savegame_level = Savegame_GetLevelNumber(slot_num);
    const bool prev_enable_legal = g_Config.gameplay.enable_legal;
    g_Config.gameplay.enable_legal = false;

    // Play intro FMVs and cutscenes
    GF_DoFrontendSequence();

    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i <= MIN(savegame_level, level_table->count); i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        if (level->type == GFL_GYM) {
            continue;
        }
        const GF_COMMAND gf_cmd = GF_InterpretSequence(
            level, GFSC_STORY, (void *)(intptr_t)savegame_level);
        if (gf_cmd.action == GF_EXIT_TO_TITLE
            || gf_cmd.action == GF_EXIT_GAME) {
            break;
        }
    }

    g_Config.gameplay.enable_legal = prev_enable_legal;
    return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
}

bool GF_HasAvailableStory(const int32_t slot_num)
{
    const int32_t savegame_level = Savegame_GetLevelNumber(slot_num);

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
                const GF_FMV *const fmv = &g_GameFlow.fmvs[fmv_id];
                if (!fmv->is_legal && !fmv->is_credit) {
                    return true;
                }
            }
        }
    }
    return false;
}

GF_COMMAND GF_DoLevelSequence(
    const GF_LEVEL *const start_level, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    const GF_LEVEL *current_level = start_level;

    const GF_LEVEL_TABLE_TYPE level_table_type =
        GF_GetLevelTableType(current_level->type);
    const int32_t level_count = GF_GetLevelTable(level_table_type)->count;
    while (true) {
        const GF_COMMAND gf_cmd =
            GF_InterpretSequence(current_level, seq_ctx, nullptr);

        if (gf_cmd.action != GF_NOOP && gf_cmd.action != GF_LEVEL_COMPLETE) {
            return gf_cmd;
        }
        if (Game_IsInGym()) {
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }
        if (current_level->num + 1 >= level_count) {
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
        }
        current_level++;
    }
}
