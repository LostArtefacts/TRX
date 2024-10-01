#include "game/console/cmd/play_level.h"

#include "game/game_string.h"
#include "game/gameflow/common.h"
#include "strings.h"

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    VECTOR *source = NULL;
    VECTOR *matches = NULL;
    int32_t level_to_load = -1;

    if (String_ParseInteger(ctx->args, &level_to_load)) {
        goto matched;
    }

    source = Vector_Create(sizeof(STRING_FUZZY_SOURCE));
    for (int32_t level_num = 0; level_num < Gameflow_GetLevelCount();
         level_num++) {
        STRING_FUZZY_SOURCE source_item = {
            .key = Gameflow_GetLevelTitle(level_num),
            .value = (void *)(intptr_t)level_num,
            .weight = 1,
        };
        Vector_Add(source, &source_item);
    }

    const int32_t gym_level_num = Gameflow_GetGymLevelNumber();
    if (gym_level_num != -1) {
        STRING_FUZZY_SOURCE source_item = {
            .key = "gym",
            .value = (void *)(intptr_t)gym_level_num,
            .weight = 1,
        };
        Vector_Add(source, &source_item);
    }

    COMMAND_RESULT result;
    matches = String_FuzzyMatch(ctx->args, source);

    if (matches->count == 0) {
        Console_Log(GS(OSD_INVALID_LEVEL));
        result = CR_BAD_INVOCATION;
        goto cleanup;
    } else if (matches->count >= 1) {
        const STRING_FUZZY_MATCH *const match = Vector_Get(matches, 0);
        level_to_load = (int32_t)(intptr_t)match->value;
        goto matched;
    }

matched:
    if (level_to_load >= 0 && level_to_load < Gameflow_GetLevelCount()) {
        Gameflow_OverrideCommand((GAMEFLOW_COMMAND) {
            .action = GF_START_GAME,
            .param = level_to_load,
        });
        Console_Log(GS(OSD_PLAY_LEVEL), Gameflow_GetLevelTitle(level_to_load));
        result = CR_SUCCESS;
    } else {
        Console_Log(GS(OSD_INVALID_LEVEL));
        result = CR_FAILURE;
    }

cleanup:
    if (matches != NULL) {
        Vector_Free(matches);
        matches = NULL;
    }
    if (source != NULL) {
        Vector_Free(source);
        source = NULL;
    }

    return result;
}

CONSOLE_COMMAND g_Console_Cmd_PlayLevel = {
    .prefix = "play|level",
    .proc = M_Entrypoint,
};
