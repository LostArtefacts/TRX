#include "game/clock.h"
#include "game/console/common.h"
#include "game/fmv.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/savegame.h"
#include "game/screen.h"
#include "game/shell.h"
#include "game/sound.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/enum_map.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/game_string_manager.h>
#include <libtrx/game/music.h>
#include <libtrx/game/option.h>
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

static void M_HandleWindowResized(void);
static void M_SetWindowPos(int32_t x, int32_t y, bool update);
static void M_SetWindowSize(int32_t width, int32_t height, bool update);
static void M_SetWindowMaximized(bool is_enabled, bool update);
static void M_SetFullscreen(bool is_enabled, bool update);
static void M_SetGLBackend(GFX_GL_BACKEND backend);

static void M_ShowHelp(void);
static void M_LoadConfig(void);
static void M_HandleConfigChange(const EVENT *event, void *data);

static void M_HandleWindowResized(void)
{
    int x;
    int y;
    int width;
    int height;
    bool is_maximized;

    Uint32 window_flags = SDL_GetWindowFlags(m_Window);
    is_maximized = window_flags & SDL_WINDOW_MAXIMIZED;
    SDL_GetWindowSize(m_Window, &width, &height);
    SDL_GetWindowPosition(m_Window, &x, &y);
    LOG_INFO("%dx%d+%d,%d (maximized: %d)", width, height, x, y, is_maximized);

    M_SetWindowMaximized(is_maximized, false);
    M_SetWindowPos(x, y, false);
    M_SetWindowSize(width, height, false);

    // save the updated config, but ensure it was loaded first
    if (g_Config.loaded) {
        Config_Write();
    }
}

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
            M_HandleWindowResized();
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

static void M_HandleConfigChange(const EVENT *const event, void *const data)
{
    const CONFIG *const old = &g_Config;
    const CONFIG *const new = &g_SavedConfig;

#define L_CHANGED(subject) (old->subject != new->subject)

    if (L_CHANGED(audio.sound_volume)) {
        Sound_SetMasterVolume(g_Config.audio.sound_volume);
    }
    if (L_CHANGED(audio.music_volume)) {
        Music_SetVolume(g_Config.audio.music_volume);
    }

    if (L_CHANGED(gameplay.maximum_save_slots) && Savegame_IsInitialised()) {
        Savegame_Shutdown();
        Savegame_Init();
        Savegame_ScanSavedGames();
        Savegame_HighlightNewestSlot();
    }

    if (L_CHANGED(language)) {
        GameStringManager_ReloadLanguage(g_Config.language);
    }

    if (L_CHANGED(window.is_fullscreen)) {
        LOG_DEBUG("Change in settings detected");
        SDL_SetWindowFullscreen(
            m_Window,
            g_Config.window.is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_ShowCursor(
            g_Config.window.is_fullscreen ? SDL_DISABLE : SDL_ENABLE);
    }

    Output_ApplyRenderSettings();
}

static void M_Start(void)
{
    M_SetFullscreen(g_Config.window.is_fullscreen, true);
    M_SetWindowPos(g_Config.window.x, g_Config.window.y, true);
    M_SetWindowSize(g_Config.window.width, g_Config.window.height, true);
    M_SetWindowMaximized(g_Config.window.is_maximized, true);
    SDL_ShowWindow(m_Window);
}

static void M_LoadConfig(void)
{
    Config_Read();
    Config_SubscribeChanges(M_HandleConfigChange, nullptr);

    Sound_SetMasterVolume(g_Config.audio.sound_volume);
    Music_SetVolume(g_Config.audio.music_volume);
}

static void M_SetWindowPos(int32_t x, int32_t y, bool update)
{
    if (x <= 0 || y <= 0) {
        return;
    }

    // only save window position if it's in windowed state.
    if (!g_Config.window.is_fullscreen && !g_Config.window.is_maximized) {
        g_Config.window.x = x;
        g_Config.window.y = y;
    }

    if (update) {
        SDL_SetWindowPosition(m_Window, x, y);
    }
}

static void M_SetWindowSize(int32_t width, int32_t height, bool update)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    // only save window size if it's in windowed state.
    if (!g_Config.window.is_fullscreen && !g_Config.window.is_maximized) {
        g_Config.window.width = width;
        g_Config.window.height = height;
    } else {
        g_Config.window.fs_width = width;
        g_Config.window.fs_height = height;
    }

    Output_SetWindowSize(width, height);

    if (update) {
        SDL_SetWindowSize(m_Window, width, height);
    }
}

static void M_SetWindowMaximized(bool is_enabled, bool update)
{
    g_Config.window.is_maximized = is_enabled;

    if (update && is_enabled) {
        SDL_MaximizeWindow(m_Window);
    }
}

static void M_SetFullscreen(bool is_enabled, bool update)
{
    g_Config.window.is_fullscreen = is_enabled;

    if (update) {
        SDL_SetWindowFullscreen(
            m_Window, is_enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_ShowCursor(is_enabled ? SDL_DISABLE : SDL_ENABLE);
    }
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

void Shell_ProcessEvents(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (Shell_ProcessCommonEvent(&event)) {
            continue;
        }

        switch (event.type) {
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                FMV_Unmute();
                Music_Unmute();
                Sound_SetMasterVolume(g_Config.audio.sound_volume);
                break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
                FMV_Mute();
                Music_Mute();
                Sound_SetMasterVolume(0);
                break;

            case SDL_WINDOWEVENT_MOVED:
            case SDL_WINDOWEVENT_RESIZED:
                M_HandleWindowResized();
                break;
            }
            break;
        }
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
    GameString_Init();
    GameStringManager_Init();
    EnumMap_Init();
    Config_Init();

    UI_Init();
    Overlay_Init();

    Input_Init();
    Sound_Init();
    Music_Init();

    M_LoadConfig();

    Clock_Init();

    M_CreateGameWindow();

    Random_Seed();

    if (!Output_Init()) {
        Shell_ExitSystem("Could not initialise video system");
        return 1;
    }
    M_Start();
    Screen_Init();

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
    GameBuf_Init();
    Console_Init();

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
    Console_Shutdown();
    Savegame_Shutdown();

    GF_Shutdown();
    Overlay_Shutdown();
    Option_Shutdown();
    Output_Shutdown();
    Input_Shutdown();
    Music_Shutdown();
    Sound_Shutdown();
    UI_Shutdown();

    GameStringManager_Shutdown();
    GameString_Shutdown();
    GameBuf_Shutdown();

    Config_Shutdown();
    EnumMap_Shutdown();
    Log_Shutdown();
}

const char *Shell_GetConfigPath(void)
{
    return "cfg/TR1X.json5";
}

const char *Shell_GetGameFlowPath(void)
{
    return m_ModPaths[m_Args.mod].game_flow_path;
}
