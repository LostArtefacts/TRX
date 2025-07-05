#include "game/fmv.h"
#include "game/game_flow.h"
#include "game/output.h"
#include "game/savegame.h"
#include "game/shell.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/enum_map.h>
#include <libtrx/game/game_string_manager.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>

typedef enum {
    M_MOD_UNKNOWN,
    M_MOD_OG,
    M_MOD_UB,
    M_MOD_DEMO_PC,
    M_MOD_CUSTOM_LEVEL,
} M_MOD;

typedef struct {
    M_MOD mod;
    const char *level_to_play;
    int32_t save_to_load;
} SHELL_ARGS;

static SDL_Window *m_Window = nullptr;
static const char *const m_CommonStringsPath = "cfg/TRX_common_strings.json5";

static struct {
    char *game_flow_path;
    char *game_strings_path;
} m_ModPaths[] = {
    [M_MOD_OG] = {
        .game_flow_path = "cfg/TR1X_gameflow.json5",
        .game_strings_path = "cfg/TR1X_strings.json5",
    },
    [M_MOD_UB] = {
        .game_flow_path = "cfg/TR1X_gameflow_ub.json5",
        .game_strings_path = "cfg/TR1X_strings_ub.json5",
    },
    [M_MOD_DEMO_PC] = {
        .game_flow_path = "cfg/TR1X_gameflow_demo_pc.json5",
        .game_strings_path = "cfg/TR1X_strings_demo_pc.json5",
    },
    [M_MOD_CUSTOM_LEVEL] = {
        .game_flow_path = "cfg/TR1X_gameflow_level.json5",
        .game_strings_path = "cfg/TR1X_strings_level.json5",
    },
};

static SHELL_ARGS m_Args = {
    .mod = M_MOD_UNKNOWN,
    .level_to_play = nullptr,
    .save_to_load = -1,
};

static void M_SetGLBackend(GFX_GL_BACKEND backend);

static void M_ShowHelp(void);

static void M_CreateGameWindow(void)
{
    SDL_Window *const window = SDL_CreateWindow(
        "TR1X", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE
            | SDL_WINDOW_OPENGL);

    if (window == nullptr) {
        Shell_ExitSystem("System Error: cannot create window");
        return;
    }

    const GFX_GL_BACKEND backends_to_try[] = {
        // clang-format off
        GFX_GL_33C,
        GFX_GL_INVALID_BACKEND, // guard
        // clang-format on
    };

    for (int32_t i = 0; backends_to_try[i] != GFX_GL_INVALID_BACKEND; i++) {
        const GFX_GL_BACKEND backend = backends_to_try[i];

        M_SetGLBackend(backend);

        int32_t major;
        int32_t minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        LOG_DEBUG("Trying GL backend %d.%d", major, minor);
        if (GFX_Context_Attach(window, backend)) {
            m_Window = window;
            return;
        }
    }

    Shell_ExitSystem("System Error: cannot attach opengl context");
}

static void M_ShowHelp(void)
{
    puts("Currently available options:");
    puts("");
    puts("-g/--gold: launch The Unfinished Business expansion pack.");
    puts("   --demo-pc: launch the PC demo level file.");
    puts("-l/--level <PATH>: launch a specific level file.");
    puts("-s/--save <NUM>: launch from a specific save slot (starts at 1).");
}

void Shell_HandleConfigChange(const CONFIG *const old, const CONFIG *const new)
{
    Shell_HandleCommonConfigChange(old, new);

#define L_CHANGED(subject) (old->subject != new->subject)

    if (L_CHANGED(rendering.upscaling_filter)
        || L_CHANGED(rendering.enable_wireframe)
        || L_CHANGED(rendering.wireframe_width)
        || L_CHANGED(rendering.enable_vsync)
        || L_CHANGED(rendering.anisotropy_filter)) {
        Output_ApplyRenderSettings();
    }

    if (L_CHANGED(gameplay.maximum_save_slots) && Savegame_IsInitialised()) {
        Savegame_Shutdown();
        Savegame_Init();
        Savegame_ScanSavedGames();
        Savegame_HighlightNewestSlot();
    }
#undef L_CHANGED
}

static void M_ShowWindow(void)
{
    Shell_SyncToWindow();
    SDL_ShowWindow(m_Window);
    SDL_RaiseWindow(m_Window);
    Shell_RefreshRendererViewport();
}

static void M_SetGLBackend(const GFX_GL_BACKEND backend)
{
    switch (backend) {
    case GFX_GL_33C:
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        break;

    case GFX_GL_INVALID_BACKEND:
        ASSERT_FAIL();
        break;
    }
}

SDL_Window *Shell_GetWindow(void)
{
    return m_Window;
}

bool Shell_ParseArgs(const int32_t arg_count, const char **args)
{
    SHELL_ARGS *const out_args = &m_Args;
    out_args->mod = M_MOD_OG;

    for (int32_t i = 0; i < arg_count; i++) {
        if (!strcmp(args[i], "-h") || !strcmp(args[i], "--help")) {
            M_ShowHelp();
            return false;
        }
        if (!strcmp(args[i], "-g") || !strcmp(args[i], "--gold")
            || !strcmp(args[i], "-gold")) {
            out_args->mod = M_MOD_UB;
        }
        if (!strcmp(args[i], "--demo-pc") || !strcmp(args[i], "-demo_pc")) {
            out_args->mod = M_MOD_DEMO_PC;
        }
        if ((!strcmp(args[i], "-l") || !strcmp(args[i], "--level"))
            && i + 1 < arg_count) {
            out_args->level_to_play = args[i + 1];
            out_args->mod = M_MOD_CUSTOM_LEVEL;
        }
        if ((!strcmp(args[i], "-s") || !strcmp(args[i], "--save"))
            && i + 1 < arg_count) {
            if (String_ParseInteger(args[i + 1], &out_args->save_to_load)) {
                out_args->save_to_load--;
            }
        }
    }
    return true;
}

int32_t Shell_Main(void)
{
    Shell_CommonInit();

    M_CreateGameWindow();

    if (!Output_Init()) {
        Shell_ExitSystem("Could not initialise video system");
        return 1;
    }
    M_ShowWindow();

    GF_Init();
    GF_LoadFromFile(m_ModPaths[m_Args.mod].game_flow_path);
    GameStringManager_ClearSourceFiles();
    GameStringManager_AddSourceFile(m_CommonStringsPath, false);
    GameStringManager_AddSourceFile(
        m_ModPaths[M_MOD_OG].game_strings_path, false);
    GameStringManager_AddSourceFile(
        m_ModPaths[m_Args.mod].game_strings_path, true);
    GameStringManager_DiscoverLanguages();
    GameStringManager_ReloadLanguage(g_Config.language);

    Savegame_Init();
    Savegame_ScanSavedGames();
    Savegame_HighlightNewestSlot();

    if (m_Args.level_to_play != nullptr) {
        Memory_Free(g_GameFlow.level_tables[GFLT_MAIN].levels[0].path);
        g_GameFlow.level_tables[GFLT_MAIN].levels[0].path =
            Memory_DupStr(m_Args.level_to_play);
    }

    GF_COMMAND gf_cmd = m_Args.save_to_load != -1
        ? (GF_COMMAND) { .action = GF_START_SAVED_GAME,
                         .param = m_Args.save_to_load }
        : m_Args.level_to_play != nullptr
        ? (GF_COMMAND) { .action = GF_START_GAME, .param = 0 }
        : GF_DoFrontendSequence();

    bool loop_continue = !Shell_IsExiting();
    while (loop_continue) {
        LOG_INFO(
            "action=%s param=%d", ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action),
            gf_cmd.param);

        switch (gf_cmd.action) {
        case GF_START_GAME:
        case GF_SELECT_GAME: {
            const int32_t level_num = gf_cmd.param;
            const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, level_num);
            const GF_SEQUENCE_CONTEXT seq_ctx =
                gf_cmd.action == GF_SELECT_GAME ? GFSC_SELECT : GFSC_NORMAL;
            if (level != nullptr) {
                gf_cmd = GF_DoLevelSequence(level, seq_ctx);
            }
            break;
        }

        case GF_START_SAVED_GAME: {
            const int16_t slot_num = gf_cmd.param;
            const int16_t level_num = Savegame_GetLevelNumber(slot_num);
            if (level_num < 0) {
                LOG_ERROR("Corrupt save file!");
                gf_cmd = (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
            } else {
                Savegame_BindSlot(slot_num);
                const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, level_num);
                gf_cmd = GF_DoLevelSequence(level, GFSC_SAVED);
            }
            break;
        }

        case GF_RESTART_GAME: {
            const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, gf_cmd.param);
            gf_cmd = GF_InterpretSequence(level, GFSC_RESTART, nullptr);
            break;
        }

        case GF_STORY_SO_FAR:
            gf_cmd = GF_PlayAvailableStory(gf_cmd.param);
            break;

        case GF_START_CINE:
            gf_cmd = GF_DoCutsceneSequence(gf_cmd.param);
            break;

        case GF_START_DEMO:
            gf_cmd = GF_DoDemoSequence(gf_cmd.param);
            break;

        case GF_NOOP:
        case GF_LEVEL_COMPLETE:
            gf_cmd = (GF_COMMAND) { .action = GF_EXIT_TO_TITLE };
            break;

        case GF_EXIT_TO_TITLE:
            if (m_Args.level_to_play != nullptr) {
                gf_cmd = (GF_COMMAND) { .action = GF_EXIT_GAME };
            } else if (g_GameFlow.title_level == nullptr) {
                Shell_ExitSystem("Title disabled");
            } else {
                gf_cmd = GF_RunTitle();
            }
            break;

        case GF_EXIT_GAME:
            loop_continue = false;
            break;

        default:
            ASSERT_FAIL_FMT(
                "invalid action (action=%s, param=%d)",
                ENUM_MAP_TO_STRING(GF_ACTION, gf_cmd.action), gf_cmd.param);
        }
    }

    Config_Write();

    if (m_Args.level_to_play != nullptr) {
        Memory_FreePointer(&g_GameFlow.level_tables[GFLT_MAIN].levels[0].path);
    }
    return 0;
}

void Shell_Shutdown(void)
{
    Shell_ShutdownCommonModules();
}

const char *Shell_GetConfigPath(void)
{
    return "cfg/TR1X.json5";
}

const char *Shell_GetGameFlowPath(void)
{
    return m_ModPaths[m_Args.mod].game_flow_path;
}

int32_t Shell_GetWindowWidth(void)
{
    return Shell_GetWindowSize().w;
}

int32_t Shell_GetWindowHeight(void)
{
    return Shell_GetWindowSize().h;
}
