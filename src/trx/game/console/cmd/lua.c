#include <trx/core/strings.h>
#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/lua/common.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx)
{
    if (String_IsEmpty(ctx->args)) {
        return CR_BAD_INVOCATION;
    }

    COMMAND_RESULT cmd_result;
    LUA_RESULT eval_result = Lua_Eval(ctx->args);
    if (eval_result.code == LUA_ERRSYNTAX) {
        Console_LogError(GS(CMD_LUA_SYNTAX_ERROR), eval_result.message);
        cmd_result = CR_FAILURE;
    } else if (eval_result.code != LUA_OK) {
        Console_LogError(GS(CMD_LUA_RUNTIME_ERROR), eval_result.message);
        cmd_result = CR_FAILURE;
    } else {
        cmd_result = CR_SUCCESS;
    }
    Lua_FreeResult(&eval_result);
    return cmd_result;
}

REGISTER_CONSOLE_COMMAND("lua", M_Entrypoint, GS_ID(CONSOLE_HELP_LUA))
