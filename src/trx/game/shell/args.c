#include <trx/game/shell/args.h>

#include <trx/debug.h>
#include <trx/memory.h>
#include <trx/strings.h>
#include <trx/utils.h>
#include <trx/version.h>

#include <stdio.h>
#include <string.h>

static void M_ShowHelp(void)
{
    puts("Currently available options:");
    puts("");
    if (Shell_GetModByName("tr1-ub") != nullptr) {
        puts("-g/--gold: launch The Unfinished Business expansion pack.");
    }
    if (Shell_GetModByName("tr1-demo-pc") != nullptr) {
        puts("   --demo-pc: launch the PC demo level file.");
    }
    if (Shell_GetModByName("tr2-gm") != nullptr) {
        puts("-g/--gold: launch The Golden Mask expansion pack.");
    }
    puts("--mod <MOD_ID>: launch a specific mod by id (e.g. tr1, tr1-ub).");
    puts("-l/--level <PATH>: launch a specific level file.");
    puts("-s/--save <NUM>: launch from a specific save slot (starts at 1).");
    puts("--test-record <PATH>: record gameplay events to file.");
    puts("--test-replay <PATH>: replay gameplay events from file.");
    puts("--headless: replay gameplay without showing a game window.");
    puts("--headless-fps: control the frame rate at which to run a gameplay.");
    puts("-q: silence logs and only show errors.");
    puts(
        "--debug-render-performance: output diagnostic information after each "
        "frame.");
}

SHELL_ARGS *Shell_ParseArgs(VECTOR *const args)
{
    SHELL_ARGS *out_args = Memory_Alloc(sizeof(SHELL_ARGS));
    out_args->save_to_load = -1;
    out_args->level_to_select = -1;
    out_args->original_args = args;
    out_args->engine_version = 0;

    // First pass: set the engine version
    for (int32_t i = 0; args != nullptr && i < args->count; i++) {
        const char *const arg = *(char **)Vector_Get(args, i);
        const char *const next_arg =
            i + 1 < args->count ? *(char **)Vector_Get(args, i + 1) : nullptr;
        if (!strcmp(arg, "-e") || !strcmp(arg, "--engine")) {
            String_ParseInteger(next_arg, &out_args->engine_version);
            CLAMP(out_args->engine_version, 1, 3);
            i++;
        }
    }

    out_args->mod = Shell_GetModByType(MOD_BASE_GAME, out_args->engine_version);

    // Second pass: remaining options
    for (int32_t i = 0; args != nullptr && i < args->count; i++) {
        const char *const arg = *(char **)Vector_Get(args, i);
        const char *const next_arg =
            i + 1 < args->count ? *(char **)Vector_Get(args, i + 1) : nullptr;

        if (!strcmp(arg, "-e") || !strcmp(arg, "--engine")) {
            i++;
        }

        if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
            M_ShowHelp();
            Memory_FreePointer(&out_args);
            return nullptr;
        }
        if (!strcmp(arg, "-g") || !strcmp(arg, "--gold")
            || !strcmp(arg, "-gold")) {
            out_args->mod = Shell_GetModByType(
                MOD_EXPANSION_PACK, out_args->engine_version);
        }

        if (!strcmp(arg, "--demo-pc") || !strcmp(arg, "-demo_pc")) {
            out_args->mod = Shell_GetModByName("tr1-demo-pc");
        }
        if (!strcmp(arg, "--mod") && next_arg != nullptr) {
            const SHELL_MOD *const mod = Shell_GetModByName(next_arg);
            if (mod != nullptr) {
                out_args->mod = mod;
            }
            i++;
        }

        if ((!strcmp(arg, "-l") || !strcmp(arg, "--level"))
            && next_arg != nullptr) {
            int32_t lvnum = -1;
            if (String_ParseInteger(next_arg, &lvnum)) {
                out_args->level_to_select = lvnum;
            } else {
                out_args->level_to_play = next_arg;
                out_args->mod = Shell_GetModByType(
                    MOD_DIRECT_LEVEL, out_args->engine_version);
            }
            i++;
        }
        if ((!strcmp(arg, "-s") || !strcmp(arg, "--save"))
            && next_arg != nullptr) {
            if (String_ParseInteger(next_arg, &out_args->save_to_load)) {
                out_args->save_to_load--;
            }
            i++;
        }
        if (!strcmp(arg, "--test-record") && next_arg != nullptr) {
            out_args->test_record_path = next_arg;
            i++;
        }
        if ((!strcmp(arg, "--test-play") || !strcmp(arg, "--test-replay"))
            && next_arg != nullptr) {
            out_args->test_replay_path = next_arg;
            i++;
        }
        if (!strcmp(arg, "--headless")) {
            out_args->headless = true;
        }
        if (!strcmp(arg, "--headless-fps") && next_arg != nullptr) {
            int32_t fps = 0;
            if (String_ParseInteger(next_arg, &fps) && fps > 0) {
                out_args->headless_fps = fps;
            }
            i++;
        }
        if (!strcmp(arg, "--debug-render-performance")) {
            out_args->debug_render_performance = true;
        }
        if (!strcmp(arg, "-q") || !strcmp(arg, "--quiet")) {
            out_args->quiet = true;
        }
    }

    return out_args;
}
