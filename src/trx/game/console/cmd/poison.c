#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/game/console/common.h>
#include <trx/game/console/registry.h>
#include <trx/game/game.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/lara/common.h>
#include <trx/version.h>

#include <string.h>

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (!Game_IsPlayable()) {
        return CR_UNAVAILABLE;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (String_IsEmpty(ctx->args)) {
        Console_Log(
            GS("general/osd/current_poison_get"), lara->poison.value,
            lara->poison.target);
        return CR_SUCCESS;
    }

    char *args = Memory_DupStr(ctx->args);
    bool set_target = false;
    char *flag = strstr(args, "-t");
    if (flag != nullptr && (flag == args || flag[-1] == ' ')
        && (flag[2] == '\0' || flag[2] == ' ')) {
        set_target = true;
        memset(flag, ' ', 2);
    }

    int32_t value;
    const bool parsed = String_ParseInteger(args, &value);
    Memory_FreePointer(&args);
    if (!parsed) {
        return CR_BAD_INVOCATION;
    }
    if (set_target && g_TRVersion != 4) {
        return CR_UNAVAILABLE;
    }

    if (set_target) {
        CLAMP(value, 0, 4096);
        lara->poison.target = value;
        Console_Log(GS("general/osd/current_poison_target_set"), value);
    } else {
        CLAMP(value, 0, g_TRVersion == 4 ? 4096 : 256);
        lara->poison.value = value;
        Console_Log(GS("general/osd/current_poison_set"), value);
    }
    return CR_SUCCESS;
}

REGISTER_CONSOLE_COMMAND(
    "poison", M_Entrypoint, GS_ID("console/cmd/poison/help"))
