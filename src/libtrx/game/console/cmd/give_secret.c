#include "debug.h"
#include "game/console/common.h"
#include "game/console/registry.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/savegame.h"
#include "game/stats/common.h"
#include "memory.h"
#include "strings.h"

#include <stdio.h>
#include <string.h>

static COMMAND_RESULT M_TakeSecret(int32_t num);
static COMMAND_RESULT M_GiveSecret(int32_t num);
static COMMAND_RESULT M_ListSecrets(void);
static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_TakeSecret(const int32_t num)
{
    if (Stats_TakeSecret(num - 1)) {
        Console_Log(GS(CMD_GIVE_SECRET_TAKEN), num);
        return CR_SUCCESS;
    }
    Console_Log(GS(CMD_INVALID_SECRET), num);
    return CR_FAILURE;
}

static COMMAND_RESULT M_GiveSecret(const int32_t num)
{
    if (Stats_AddSecret(num - 1)) {
        Console_Log(GS(CMD_GIVE_SECRET_GIVEN), num);
        return CR_SUCCESS;
    }
    Console_Log(GS(CMD_INVALID_SECRET), num);
    return CR_FAILURE;
}

static COMMAND_RESULT M_ListSecrets(void)
{
    RESUME_INFO *const info = Savegame_GetCurrentInfo(Game_GetCurrentLevel());
    ASSERT(info != nullptr);
    char buf[128] = { 0 };
    char *ptr = buf;
    bool first = true;
    for (int32_t num = 1; num <= info->stats.max_secret_count; num++) {
        if (Stats_HasSecret(num - 1)) {
            if (!first) {
                ptr += sprintf(ptr, ", ");
            }
            first = false;
            ptr += sprintf(ptr, "%d", num);
        }
    }
    if (strcmp(buf, "") == 0) {
        Console_Log(GS(CMD_GIVE_SECRET_NONE));
    } else {
        Console_Log(GS(CMD_GIVE_SECRET_LIST), buf);
    }
    return CR_SUCCESS;
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!Game_IsPlayable()) {
        return CR_UNAVAILABLE;
    }

    if (String_IsEmpty(ctx->args)) {
        return M_ListSecrets();
    }

    char *args = Memory_DupStr(ctx->args);
    char *subcmd = args;
    char *param = nullptr;

    if (!String_IsEmpty(ctx->args)) {
        param = strchr(args, ' ');
        if (param != nullptr) {
            *param = '\0';
            param++;
        }
    }

    if (param == nullptr || String_IsEmpty(param)) {
        Memory_FreePointer(&args);
        return CR_BAD_INVOCATION;
    }

    int32_t num;
    if (!String_ParseInteger(param, &num)) {
        Memory_FreePointer(&args);
        return CR_BAD_INVOCATION;
    }

    COMMAND_RESULT result = CR_FAILURE;
    if (String_Equivalent(subcmd, "take")) {
        result = M_TakeSecret(num);
    } else if (String_Equivalent(subcmd, "give")) {
        result = M_GiveSecret(num);
    } else {
        result = CR_BAD_INVOCATION;
    }

    Memory_FreePointer(&args);
    return result;
}

REGISTER_CONSOLE_COMMAND(
    "secret", M_Entrypoint, GS_ID(CONSOLE_HELP_GIVE_SECRET))
