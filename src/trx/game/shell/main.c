#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/game/shell.h>
#include <trx/game/shell/common.h>
#include <trx/game/shell/mod.h>
#include <trx/version.h>

#include <string.h>

int main(int argc, char *argv[])
{
    VECTOR *raw_args = Vector_Create(sizeof(const char *));
    for (int32_t i = 1; i < argc; i++) {
        char *const copied_arg = Memory_DupStr(argv[i]);
        Vector_Add(raw_args, &copied_arg);
    }

    GamePath_Init(nullptr);
    EXIT_ON_FAIL(Shell_ScanAvailableMods(), "TRX cannot find the games");
    SHELL_ARGS *args = nullptr;
    EXIT_ON_FAIL(
        Shell_ParseArgs(raw_args, &args),
        "TRX cannot make sense of how it was started");
    if (args == nullptr) {
        return 0;
    }

    GamePath_Init(args);

    char *log_path =
        String_Format("%s/TRX.log", GamePath_Get(GAME_PATH_TRX_DIR));
    Log_Init(log_path, args->quiet ? LOG_LEVEL_WARNING : LOG_LEVEL_MAX);
    Memory_FreePointer(&log_path);

    LOG_INFO("Starting %s", g_TRXVersion);
    Shell_ValidateMods();
    // A game the player named is theirs to hear about, so it stays where it is
    // for the shell to report. Anything else falls back to what can be played.
    if (!args->startup.mod_explicit
        && (args->startup.mod == nullptr || !args->startup.mod->is_valid)) {
        args->startup.mod =
            Shell_SelectStartupMod(args->startup.engine_version);
        if (args->startup.mod != nullptr && args->startup.engine_version == 0) {
            args->startup.engine_version = args->startup.mod->engine_version;
        }
    }

    int32_t exit_code;
    bool restart;
    do {
        GamePath_Init(args);
        restart = false;
        exit_code = Shell_Main(args);
        // Note: on a mod switch, Shell_Main has already freed args (via the
        // session) and reset m_Session to nullptr. Do not touch args after
        // this point in the restart branch.

        const char *const pending_mod = Shell_GetPendingMod();
        if (pending_mod != nullptr) {
            const SHELL_MOD *const mod = Shell_GetModByName(pending_mod);
            Shell_ClearPendingMod();
            if (mod != nullptr && mod->is_available) {
                LOG_INFO("Switching mod to: %s", mod->name);
                SHELL_ARGS *const next_args = Memory_Alloc(sizeof(SHELL_ARGS));
                *next_args = (SHELL_ARGS) {
                    .startup = {
                        .engine_version = mod->engine_version,
                        .mod = mod,
                        .level_request = { .num = -1 },
                        .save_to_load = -1,
                    },
                    .headless = Shell_GetPrevHeadless(),
                    .quiet = Shell_GetPrevQuiet(),
                };
                args = next_args;
                GamePath_Init(args);
                restart = true;
            }
        }
    } while (restart);

    Shell_Terminate(exit_code);
    return exit_code;
}
