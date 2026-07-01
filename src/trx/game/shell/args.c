#include <trx/game/shell/args.h>

#include <trx/core/filesystem.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/core/virtual_file.h>
#include <trx/debug.h>
#include <trx/game/level/format/format.h>
#include <trx/game/shell/common.h>
#include <trx/game/shell/paths.h>
#include <trx/version.h>

#include <stdio.h>
#include <string.h>

static void M_FreeArgVector(VECTOR *const args)
{
    if (args == nullptr) {
        return;
    }

    for (int32_t i = 0; i < args->count; i++) {
        const char *const arg = *(char **)Vector_Get(args, i);
        Memory_Free((char *)arg);
    }
    Vector_Free(args);
}

static void M_ShowHelp(void)
{
    puts("Available options:");
    puts("");
    puts("-h/--help: show this help.");
    puts("   --mod <MOD_ID>: launch a specific game or mod directly.");
    puts("-e/--engine <1|2|3|4>: pick a game engine explicitly.");
    puts("-l/--level <PATH|NUM>: launch a level file or level number.");
    puts("-s/--save <NUM>: launch from a specific save slot (starts at 1).");
    puts("   --test-record <PATH>: record gameplay events to file.");
    puts(
        "   --test-replay/--test-play <PATH>: replay gameplay events from "
        "file.");
    puts("   --headless: replay gameplay without showing a game window.");
    puts(
        "   --headless-fps <NUM>: control replay frame rate in headless mode.");
    puts("-q/--quiet: silence logs and only show errors.");
    puts(
        "   --debug-render-performance: output diagnostic information after "
        "each "
        "frame.");
    puts("");

    puts("Available mods:");
    for (int32_t i = 0; i < Shell_GetModCount(); i++) {
        const SHELL_MOD *const mod = Shell_GetMod(i);
        if (mod == nullptr || !mod->is_available) {
            continue;
        }
        if (mod->mod_type == MOD_DIRECT_LEVEL) {
            continue;
        }
        if (mod->title != nullptr && strcmp(mod->title, mod->name) != 0) {
            printf("  %s (%s)\n", mod->name, mod->title);
        } else {
            printf("  %s\n", mod->name);
        }
    }
    puts("");

    puts("Legacy options:");
    puts("-g/--gold/-gold: launch the matching Gold expansion pack.");
    if (Shell_GetModByName("tr1-demo-pc") != nullptr) {
        puts("   --demo-pc/-demo_pc: launch the TR1 PC demo.");
    }
    puts("These options are deprecated; please use --mod instead.");
}

static int32_t M_GuessEngineVersionFromLevelPath(const char *const path)
{
    if (path == nullptr || !File_Exists(path)) {
        return 0;
    }

    VFILE *const file = VFile_CreateFromPath(path);
    if (file == nullptr) {
        return 0;
    }

    const LEVEL_FORMAT_LOADER *const loader = Level_Format_GuessLoader(file);
    const int32_t game_version = loader != nullptr ? loader->game_version : 0;
    VFile_Close(file);
    return game_version;
}

SHELL_ARGS *Shell_ParseArgs(VECTOR *const args)
{
    SHELL_ARGS *const result = Memory_Alloc(sizeof(SHELL_ARGS));
    bool wants_gold = false;
    bool explicit_engine_version = false;
    result->startup.save_to_load = -1;
    result->startup.level_to_select = -1;
    result->original_args = args;
    result->startup.engine_version = 0;

    // First pass: set the engine version.
    for (int32_t i = 0; args != nullptr && i < args->count; i++) {
        const char *const arg = *(char **)Vector_Get(args, i);
        const char *const next_arg =
            i + 1 < args->count ? *(char **)Vector_Get(args, i + 1) : nullptr;
        if (!strcmp(arg, "-e") || !strcmp(arg, "--engine")) {
            String_ParseInteger(next_arg, &result->startup.engine_version);
            CLAMP(result->startup.engine_version, 1, TR_VERSION_COUNT);
            explicit_engine_version = true;
            i++;
        }
    }
    if (result->startup.engine_version <= 0 && g_TRVersion > 0) {
        // Hydrate recordings using old-style directory tree to use
        // runtime engine version if they miss it.
        result->startup.engine_version = g_TRVersion;
    }

    // Second pass: remaining options.
    for (int32_t i = 0; args != nullptr && i < args->count; i++) {
        const char *const arg = *(char **)Vector_Get(args, i);
        const char *const next_arg =
            i + 1 < args->count ? *(char **)Vector_Get(args, i + 1) : nullptr;

        if (!strcmp(arg, "-e") || !strcmp(arg, "--engine")) {
            i++;
        }

        if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
            M_ShowHelp();
            Shell_FreeArgs(result);
            return nullptr;
        }
        if (!strcmp(arg, "-g") || !strcmp(arg, "--gold")
            || !strcmp(arg, "-gold")) {
            wants_gold = true;
        }

        if (!strcmp(arg, "--demo-pc") || !strcmp(arg, "-demo_pc")) {
            result->startup.mod = Shell_GetModByName("tr1-demo-pc");
        }
        if (!strcmp(arg, "--mod") && next_arg != nullptr) {
            const SHELL_MOD *const mod = Shell_GetModByName(next_arg);
            if (mod != nullptr) {
                result->startup.mod = mod;
            }
            i++;
        }

        if ((!strcmp(arg, "-l") || !strcmp(arg, "--level"))
            && next_arg != nullptr) {
            int32_t lvnum = -1;
            if (String_ParseInteger(next_arg, &lvnum)) {
                result->startup.level_to_select = lvnum;
                if (result->startup.mod == nullptr
                    && result->startup.engine_version > 0) {
                    result->startup.mod = Shell_GetModByType(
                        MOD_BASE_GAME, result->startup.engine_version);
                }
            } else {
                char **const level_arg = Vector_Get(args, i + 1);
                ASSERT(level_arg != nullptr);

                const char *const resolved_level_path =
                    TRXPath_PeekResolveUserPath(
                        TRX_DYNAMIC_PATH_LEVEL_FILE, next_arg);
                result->startup.level_to_play = resolved_level_path != nullptr
                    ? Memory_DupStr(resolved_level_path)
                    : nullptr;
                if (result->startup.level_to_play == nullptr) {
                    Shell_ExitSystemFmt(
                        "Cannot find level file '%s'. Relative paths are "
                        "resolved from the current working directory, then "
                        "from the game directory.",
                        next_arg);
                }
                Memory_Free(*level_arg);
                *level_arg = (char *)result->startup.level_to_play;

                if (result->startup.engine_version == 0) {
                    result->startup.engine_version =
                        M_GuessEngineVersionFromLevelPath(
                            result->startup.level_to_play);
                }
                if (result->startup.engine_version == 0) {
                    Shell_ExitSystem(
                        "Cannot determine engine version for --level. "
                        "Please provide --engine.");
                }
                result->startup.mod = Shell_GetModByType(
                    MOD_DIRECT_LEVEL, result->startup.engine_version);
                if (result->startup.mod == nullptr) {
                    Shell_ExitSystemFmt(
                        "Engine %d does not support --level with a file path "
                        "because no direct-level mod is available for that "
                        "engine.",
                        result->startup.engine_version);
                }
            }
            i++;
        }
        if ((!strcmp(arg, "-s") || !strcmp(arg, "--save"))
            && next_arg != nullptr) {
            if (String_ParseInteger(next_arg, &result->startup.save_to_load)) {
                result->startup.save_to_load--;
            }
            i++;
        }
        if (!strcmp(arg, "--test-record") && next_arg != nullptr) {
            result->test_record_path = next_arg;
            i++;
        }
        if ((!strcmp(arg, "--test-play") || !strcmp(arg, "--test-replay"))
            && next_arg != nullptr) {
            result->test_replay_path = next_arg;
            i++;
        }
        if (!strcmp(arg, "--headless")) {
            result->headless = true;
        }
        if (!strcmp(arg, "--headless-fps") && next_arg != nullptr) {
            int32_t fps = 0;
            if (String_ParseInteger(next_arg, &fps) && fps > 0) {
                result->headless_fps = fps;
            }
            i++;
        }
        if (!strcmp(arg, "--debug-render-performance")) {
            result->debug_render_performance = true;
        }
        if (!strcmp(arg, "-q") || !strcmp(arg, "--quiet")) {
            result->quiet = true;
        }
    }

    if (result->startup.mod == nullptr) {
        result->startup.mod =
            Shell_SelectStartupMod(result->startup.engine_version);
    }
    if (!explicit_engine_version && result->startup.mod != nullptr) {
        result->startup.engine_version = result->startup.mod->engine_version;
    }
    if (wants_gold) {
        const int32_t engine_version = result->startup.engine_version != 0
            ? result->startup.engine_version
            : (result->startup.mod != nullptr
                   ? result->startup.mod->engine_version
                   : 0);
        const SHELL_MOD *const gold_mod =
            Shell_GetModByType(MOD_EXPANSION_PACK, engine_version);
        if (gold_mod != nullptr) {
            result->startup.mod = gold_mod;
            result->startup.engine_version = gold_mod->engine_version;
        }
    }

    return result;
}

void Shell_FreeArgs(SHELL_ARGS *const args)
{
    if (args == nullptr) {
        return;
    }

    M_FreeArgVector(args->original_args);
    args->original_args = nullptr;
    Memory_Free(args);
}
