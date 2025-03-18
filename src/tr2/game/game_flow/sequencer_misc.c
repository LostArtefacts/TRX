#include "decomp/savegame.h"
#include "game/game.h"
#include "game/game_flow/common.h"
#include "game/game_flow/sequencer.h"
#include "game/game_flow/vars.h"
#include "game/level.h"

#include <libtrx/game/game_string_table.h>
#include <libtrx/log.h>

#include <stddef.h>

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

        if (g_GameFlow.single_level >= 0) {
            return gf_cmd;
        }
        if (gf_cmd.action != GF_NOOP && gf_cmd.action != GF_LEVEL_COMPLETE) {
            return gf_cmd;
        }
        if (g_GameFlow.is_demo_version && g_GameFlow.single_level) {
            return (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
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
