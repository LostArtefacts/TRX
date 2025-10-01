#include "game/shell/args.h"

#include "debug.h"
#include "memory.h"
#include "strings.h"

#include <stdio.h>
#include <string.h>

#if TR_VERSION == 1
    #define M_BASE_MOD SHELL_MOD_TR1_OG
#elif TR_VERSION == 2
    #define M_BASE_MOD SHELL_MOD_TR2_OG
#endif

static const char *const m_CommonStringsPath = "cfg/base_strings.json5";

static const char *m_ModDirs[] = {
    [SHELL_MOD_TR1_OG] = "cfg/tr1",
    [SHELL_MOD_TR1_UB] = "cfg/tr1-ub",
    [SHELL_MOD_TR1_DEMO_PC] = "cfg/tr1-demo-pc",
    [SHELL_MOD_TR1_CUSTOM_LEVEL] = "cfg/tr1-level",
    [SHELL_MOD_TR2_OG] = "cfg/tr2",
    [SHELL_MOD_TR2_GM] = "cfg/tr2-gm",
    [SHELL_MOD_TR2_CUSTOM_LEVEL] = "cfg/tr2-level",
};

static void M_ShowHelp(void)
{
    puts("Currently available options:");
    puts("");
#if TR_VERSION == 1
    puts("-g/--gold: launch The Unfinished Business expansion pack.");
    puts("   --demo-pc: launch the PC demo level file.");
#elif TR_VERSION == 2
    puts("-g/--gold: launch The Golden Mask expansion pack.");
#endif
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
    out_args->mod = M_BASE_MOD;
    out_args->save_to_load = -1;
    out_args->level_to_select = -1;
    out_args->original_args = args;
    if (args == nullptr) {
        return out_args;
    }

    for (int32_t i = 0; i < args->count; i++) {
        const char *const arg = *(char **)Vector_Get(args, i);
        const char *const next_arg =
            i + 1 < args->count ? *(char **)Vector_Get(args, i + 1) : nullptr;

        if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
            M_ShowHelp();
            Memory_FreePointer(&out_args);
            return nullptr;
        }
        if (!strcmp(arg, "-g") || !strcmp(arg, "--gold")
            || !strcmp(arg, "-gold")) {
            out_args->mod =
                TR_VERSION == 1 ? SHELL_MOD_TR1_UB : SHELL_MOD_TR2_GM;
        }
#if TR_VERSION == 1
        if (!strcmp(arg, "--demo-pc") || !strcmp(arg, "-demo_pc")) {
            out_args->mod = SHELL_MOD_TR1_DEMO_PC;
        }
#endif
        if ((!strcmp(arg, "-l") || !strcmp(arg, "--level"))
            && next_arg != nullptr) {
            int32_t lvnum = -1;
            if (String_ParseInteger(next_arg, &lvnum)) {
                out_args->level_to_select = lvnum;
            } else {
                out_args->level_to_play = next_arg;
                out_args->mod = TR_VERSION == 1 ? SHELL_MOD_TR1_CUSTOM_LEVEL
                                                : SHELL_MOD_TR2_CUSTOM_LEVEL;
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

const char *Shell_GetCommonStringsPath(void)
{
    return m_CommonStringsPath;
}

const char *Shell_GetBaseGameStringsPath(void)
{
    return String_FormatStatic("%s/strings.json5", m_ModDirs[M_BASE_MOD]);
}

const char *Shell_GetGameStringsPath(const SHELL_MOD mod)
{
    return String_FormatStatic("%s/strings.json5", m_ModDirs[mod]);
}

const char *Shell_GetGameFlowPath(const SHELL_MOD mod)
{
    return String_FormatStatic("%s/gameflow.json5", m_ModDirs[mod]);
}
