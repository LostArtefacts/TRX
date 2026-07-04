#include <trx/core/strings.h>
#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/game_strings/entries.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    int32_t level_num = -1;
    const GF_LEVEL *const level = String_ParseInteger(ctx->args, &level_num)
        ? GF_GetLevelByOrdinalNumber(GFLT_MAIN, level_num)
        : GF_FindPlayableLevelByQuery(ctx->args);

    if (level == nullptr || level->type == GFL_DUMMY
        || level->type == GFL_CURRENT) {
        Console_LogError(GS("general/osd/invalid_level"));
        return CR_FAILURE;
    }

    GF_OverrideCommand((GF_COMMAND) {
        .action = GF_SELECT_GAME,
        .param = level->num,
    });
    Console_Log(GS("general/osd/play_level"), level->title);
    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND(
    "play", M_Entrypoint, GS_ID("console/cmd/play_level/help"))
REGISTER_CONSOLE_COMMAND(
    "level", M_Entrypoint, GS_ID("console/cmd/play_level/help"))
