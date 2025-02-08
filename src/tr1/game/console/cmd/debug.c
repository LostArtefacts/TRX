#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/console/cmd/config.h>
#include <libtrx/game/console/registry.h>
#include <libtrx/game/game_string.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>

static bool *const m_AllOptions[] = {
    &g_Config.rendering.enable_debug_portals,
    &g_Config.rendering.enable_debug_triggers,
    nullptr,
};

static void M_Toggle(const bool enable);
static void M_ShowStatus(void);
static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static void M_Toggle(const bool enable)
{
    for (int32_t i = 0; m_AllOptions[i] != nullptr; i++) {
        void *const target = m_AllOptions[i];
        const CONFIG_OPTION *const option =
            Console_Cmd_Config_GetOptionFromTarget(target);
        char *const name = Console_Cmd_Config_NormalizeKey(option->name);
        *(bool *)target = enable;
        char value_repr[128];
        ASSERT(Console_Cmd_Config_GetCurrentValue(option, value_repr, 128));
        Console_Log(GS(OSD_CONFIG_OPTION_SET), name, value_repr);
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
        char value_repr[128];
        ASSERT(Console_Cmd_Config_GetCurrentValue(option, value_repr, 128));
        Console_Log(GS(OSD_CONFIG_OPTION_GET), name, value_repr);
        Memory_Free(name);
    }
}

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    if (String_Match(ctx->args, "^(on|true|1)$")) {
        M_Toggle(true);
        return CR_SUCCESS;
    } else if (String_Match(ctx->args, "^(off|false|0)$")) {
        M_Toggle(false);
        Config_Write();
        return CR_SUCCESS;
    } else if (String_IsEmpty(ctx->args)) {
        M_ShowStatus();
        return CR_SUCCESS;
    } else {
        return CR_BAD_INVOCATION;
    }
}

REGISTER_CONSOLE_COMMAND("debug", M_Entrypoint)
