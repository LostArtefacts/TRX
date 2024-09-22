#include "game/console/cmd/easy_config.h"

#include "config.h"

#include <libtrx/game/console/cmd/config.h>
#include <libtrx/strings.h>

typedef struct {
    const char *prefix;
    void *target;
} COMMAND_TO_OPTION_MAP;

static COMMAND_TO_OPTION_MAP m_CommandToOptionMap[] = {
    { "braid", &g_Config.enable_braid },
    { "cheats", &g_Config.enable_cheats },
    { "vsync", &g_Config.rendering.enable_vsync },
    { "wireframe", &g_Config.rendering.enable_wireframe },
    { "fps", &g_Config.rendering.fps },
    { NULL, NULL },
};

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *ctx);

static COMMAND_RESULT M_Entrypoint(const COMMAND_CONTEXT *const ctx)
{
    COMMAND_TO_OPTION_MAP *match = m_CommandToOptionMap;
    while (match->target != NULL) {
        if (String_Equivalent(match->prefix, ctx->prefix)) {
            return Console_Cmd_Config_Helper(
                Console_Cmd_Config_GetOptionFromTarget(match->target),
                ctx->args);
        }
        match++;
    }

    return CR_FAILURE;
}

CONSOLE_COMMAND g_Console_Cmd_EasyConfig = {
    .prefix = "braid|cheats|fps|vsync|wireframe",
    .proc = M_Entrypoint,
};
