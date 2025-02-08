#include <libtrx/config.h>
#include <libtrx/game/console/cmd/config.h>
#include <libtrx/game/console/registry.h>
#include <libtrx/strings.h>

typedef struct {
    const char *prefix;
    void *target;
} COMMAND_TO_OPTION_MAP;

static COMMAND_TO_OPTION_MAP m_CommandToOptionMap[] = {
    { "braid", &g_Config.visuals.enable_braid },
    { "cheats", &g_Config.gameplay.enable_cheats },
    { "vsync", &g_Config.rendering.enable_vsync },
    { "wireframe", &g_Config.rendering.enable_wireframe },
    { "fps", &g_Config.rendering.fps },
    { nullptr, nullptr },
};

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    COMMAND_TO_OPTION_MAP *match = m_CommandToOptionMap;
    while (match->target != nullptr) {
        if (String_Equivalent(match->prefix, ctx->prefix)) {
            return Console_Cmd_Config_Helper(
                Console_Cmd_Config_GetOptionFromTarget(match->target),
                ctx->args);
        }
        match++;
    }

    return CR_FAILURE;
}

REGISTER_CONSOLE_COMMAND("braid|cheats|vsync|wireframe|fps", M_Entrypoint)
