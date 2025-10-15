#include "config.h"
#include "debug.h"
#include "game/console/cmd/config.h"
#include "game/console/registry.h"
#include "game/game_string.h"
#include "memory.h"
#include "strings.h"

static bool *const m_AllOptions[] = {
    &g_Config.debug.enable_debug_portals,
    &g_Config.debug.enable_debug_room_clip,
    &g_Config.debug.enable_debug_triggers,
    &g_Config.debug.enable_debug_spheres,
    &g_Config.debug.enable_debug_cuboids,
    &g_Config.debug.enable_debug_pos,
    nullptr,
};

static void M_Toggle(const bool enable)
{
    for (int32_t i = 0; m_AllOptions[i] != nullptr; i++) {
        void *const target = m_AllOptions[i];
        const CONFIG_OPTION *const option =
            Console_Cmd_Config_GetOptionFromTarget(target);
        char *const name = Console_Cmd_Config_NormalizeKey(option->name);
        *(bool *)target = enable;
        const char *const value_str = Config_GetOptionValueAsString(option);
        ASSERT(value_str != nullptr);
        Console_Log(GS(OSD_CONFIG_OPTION_SET), name, value_str);
        Memory_Free(name);
    }
}

static void M_ShowStatus(void)
{
    for (int32_t i = 0; m_AllOptions[i] != nullptr; i++) {
        void *const target = m_AllOptions[i];
        const CONFIG_OPTION *const option =
            Console_Cmd_Config_GetOptionFromTarget(target);
        char *const name = Console_Cmd_Config_NormalizeKey(option->name);
        const char *const value_str = Config_GetOptionValueAsString(option);
        ASSERT(value_str != nullptr);
        Console_Log(GS(OSD_CONFIG_OPTION_GET), name, value_str);
        Memory_Free(name);
    }
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (String_Match(ctx->args, "^(on|true|1)$")) {
        M_Toggle(true);
        Config_Update();
        return CR_SUCCESS;
    } else if (String_Match(ctx->args, "^(off|false|0)$")) {
        M_Toggle(false);
        Config_Update();
        return CR_SUCCESS;
    } else if (String_IsEmpty(ctx->args)) {
        M_ShowStatus();
        return CR_SUCCESS;
    } else {
        return CR_BAD_INVOCATION;
    }
}

REGISTER_CONSOLE_COMMAND("debug", M_Entrypoint, GS_ID(CONSOLE_HELP_DEBUG))
