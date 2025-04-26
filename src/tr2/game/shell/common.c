#include "game/shell/common.h"

#include "decomp/decomp.h"
#include "game/clock.h"
#include "game/console/common.h"
#include "game/demo.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/level.h"
#include "game/music.h"
#include "game/objects/creatures/big_spider.h"
#include "game/objects/creatures/monk.h"
#include "game/objects/creatures/spider.h"
#include "game/output.h"
#include "game/phase.h"
#include "game/random.h"
#include "game/render/common.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/text.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/enum_map.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/game/objects/creatures/bear.h>
#include <libtrx/game/objects/creatures/wolf.h>
#include <libtrx/game/shell.h>
#include <libtrx/game/ui.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>

#include <SDL2/SDL.h>
#include <stdarg.h>
#include <stdio.h>

typedef enum {
    M_MOD_UNKNOWN,
    M_MOD_OG,
    M_MOD_GM,
    M_MOD_CUSTOM_LEVEL,
} M_MOD;

typedef struct {
    M_MOD mod;
    const char *level_to_play;
    int32_t save_to_load;
} SHELL_ARGS;

static struct {
    char *game_flow_path;
    char *game_strings_path;
} m_ModPaths[] = {
    [M_MOD_OG] = {
        .game_flow_path = "cfg/TR2X_gameflow.json5",
        .game_strings_path = "cfg/TR2X_strings.json5",
    },
    [M_MOD_GM] = {
        .game_flow_path = "cfg/TR2X_gameflow_gm.json5",
        .game_strings_path = "cfg/TR2X_strings_gm.json5",
    },
    [M_MOD_CUSTOM_LEVEL] = {
        .game_flow_path = "cfg/TR2X_gameflow_level.json5",
        .game_strings_path = "cfg/TR2X_strings_level.json5",
    },
};

static SHELL_ARGS m_Args = {
    .mod = M_MOD_UNKNOWN,
    .level_to_play = nullptr,
    .save_to_load = -1,
};

static SHELL_SIZE m_ViewportSize = { .w = -1, .h = -1 };
static Uint64 m_UpdateDebounce = 0;
static bool m_IgnoreConfigChanges = false;

static void M_SyncToWindow(void);
static void M_SyncFromWindow(bool update_viewport);
static bool M_MustUpdateRendererViewport(void);
static void M_RefreshRendererViewport(void);
static void M_HandleFocusGained(void);
static void M_HandleFocusLost(void);
static void M_HandleWindowShown(void);
static void M_HandleWindowRestored(void);
static void M_HandleWindowMinimized(void);
static void M_HandleWindowMaximized(void);
static void M_HandleWindowMoved(int32_t x, int32_t y);
static void M_HandleWindowResized(int32_t width, int32_t height);
static void M_HandleKeyDown(const SDL_Event *event);
static void M_HandleKeyUp(const SDL_Event *event);
static void M_HandleQuit(void);
static void M_ConfigureOpenGL(void);
static bool M_CreateGameWindow(void);

static void M_ShowHelp(void);
static bool M_ParseArgs(SHELL_ARGS *out_args);
static void M_LoadConfig(void);
static void M_HandleConfigChange(const EVENT *event, void *data);

static struct {
    bool is_fullscreen;
    bool is_maximized;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} m_LastWindowState = {};

static void M_SyncToWindow(void)
{
    m_UpdateDebounce = SDL_GetTicks();

    LOG_DEBUG(
        "is_fullscreen=%d is_maximized=%d x=%d y=%d width=%d height=%d",
        g_Config.window.is_fullscreen, g_Config.window.is_maximized,
        g_Config.window.x, g_Config.window.y, g_Config.window.width,
        g_Config.window.height);

    if (g_Config.window.is_fullscreen) {
        SDL_SetWindowFullscreen(g_SDLWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowCursor(SDL_DISABLE);
    } else if (g_Config.window.is_maximized) {
        SDL_SetWindowFullscreen(g_SDLWindow, 0);
        SDL_MaximizeWindow(g_SDLWindow);
        SDL_ShowCursor(SDL_ENABLE);
    } else {
        int32_t x = g_Config.window.x;
        int32_t y = g_Config.window.y;
        int32_t width = g_Config.window.width;
        int32_t height = g_Config.window.height;
        if (width <= 0 || height <= 0) {
            width = 1280;
            height = 720;
        }

        // Handle default position
        if (x == -1 && y == -1) {
            SDL_DisplayMode display_mode;
            SDL_GetCurrentDisplayMode(0, &display_mode);
            x = (display_mode.w - width) / 2;
            y = (display_mode.h - height) / 2;
        } else {
            // Adjust window position if completely offscreen
            bool on_screen = false;
            const int32_t num_displays = SDL_GetNumVideoDisplays();
            for (int32_t i = 0; i < num_displays; i++) {
                SDL_Rect bounds;
                SDL_GetDisplayBounds(i, &bounds);
                if (x + width > bounds.x && x < bounds.x + bounds.w
                    && y + height > bounds.y && y < bounds.y + bounds.h) {
                    on_screen = true;
                    break;
                }
            }
            if (!on_screen) {
                x = 0;
                y = 0;
                // Find the first display to reposition the window
                SDL_Rect bounds;
                SDL_GetDisplayBounds(0, &bounds);
                x = bounds.x + (bounds.w - width) / 2;
                y = bounds.y + (bounds.h - height) / 2;
            }
        }

        SDL_SetWindowFullscreen(g_SDLWindow, 0);
        SDL_SetWindowPosition(g_SDLWindow, x, y);
        SDL_SetWindowSize(g_SDLWindow, width, height);
        SDL_ShowCursor(SDL_ENABLE);
    }
}

static void M_SyncFromWindow(const bool update_viewport)
{
    // Determine if this call should sync config, i.e., skip immediate
    // programmatic events
    const Uint32 now = SDL_GetTicks();
    const bool skip_config = (now - m_UpdateDebounce) < 500;

    // Always pull current window state for logging and viewport reset
    const Uint32 window_flags = SDL_GetWindowFlags(g_SDLWindow);
    const bool is_maximized = window_flags & SDL_WINDOW_MAXIMIZED;
    int32_t x, y;
    int32_t width, height;
    SDL_GetWindowSize(g_SDLWindow, &width, &height);
    SDL_GetWindowPosition(g_SDLWindow, &x, &y);
    LOG_INFO("%dx%d+%d,%d (maximized: %d)", width, height, x, y, is_maximized);

    // Update config only when not in debounce window
    if (!skip_config) {
        g_Config.window.is_maximized = is_maximized;
        if (!is_maximized && !g_Config.window.is_fullscreen) {
            g_Config.window.x = x;
            g_Config.window.y = y;
            g_Config.window.width = width;
            g_Config.window.height = height;
        }
        if (g_Config.loaded) {
            m_IgnoreConfigChanges = true;
            Config_Write();
            m_IgnoreConfigChanges = false;
        }
    }

    if (update_viewport || M_MustUpdateRendererViewport()) {
        // Refresh viewport to reflect the actual window size
        M_RefreshRendererViewport();
    }
}

static bool M_MustUpdateRendererViewport(void)
{
    const SHELL_SIZE size = Shell_GetCurrentSize();
    return m_ViewportSize.w != size.w || m_ViewportSize.h != size.h;
}

static void M_RefreshRendererViewport(void)
{
    Viewport_Reset();
    m_ViewportSize = Shell_GetCurrentSize();
}

static void M_HandleFocusGained(void)
{
}

static void M_HandleFocusLost(void)
{
}

static void M_HandleWindowShown(void)
{
    LOG_DEBUG("");
}

static void M_HandleWindowRestored(void)
{
    M_SyncFromWindow(true);
}

static void M_HandleWindowMinimized(void)
{
    LOG_DEBUG("");
}

static void M_HandleWindowMaximized(void)
{
    M_SyncFromWindow(true);
}

static void M_HandleWindowMoved(const int32_t x, const int32_t y)
{
    M_SyncFromWindow(false);
}

static void M_HandleWindowResized(int32_t width, int32_t height)
{
    M_SyncFromWindow(true);
}

static void M_HandleKeyDown(const SDL_Event *const event)
{
    // NOTE: This normally would get handled by Input_Update,
    // but by the time Input_Update gets ran, we may already have lost
    // some keypresses if the player types really fast, so we need to
    // react sooner.
    if (!FMV_IsPlaying() && g_Config.gameplay.enable_console
        && !Console_IsOpened() && !Input_IsInListenMode()
        && Input_IsPressed(
            INPUT_BACKEND_KEYBOARD, g_Config.input.keyboard_layout,
            INPUT_ROLE_ENTER_CONSOLE)) {
        Console_Open();
    } else {
        UI_HandleKeyDown(event->key.keysym.sym);
    }
}

static void M_HandleKeyUp(const SDL_Event *const event)
{
    // NOTE: needs special handling on Windows -
    // SDL_SCANCODE_PRINTSCREEN is not sufficient to react to this.
    if (event->key.keysym.sym == SDLK_PRINTSCREEN) {
        Screenshot_Make(g_Config.rendering.screenshot_format);
    }
}

static void M_HandleQuit(void)
{
    Shell_ScheduleExit();
}

static void M_ConfigureOpenGL(void)
{
    // Setup minimum properties of GL context
    struct {
        SDL_GLattr attr;
        int value;
    } attrs[] = {
        { SDL_GL_RED_SIZE, 8 },     { SDL_GL_RED_SIZE, 8 },
        { SDL_GL_GREEN_SIZE, 8 },   { SDL_GL_BLUE_SIZE, 8 },
        { SDL_GL_ALPHA_SIZE, 8 },   { SDL_GL_DEPTH_SIZE, 24 },
        { SDL_GL_DOUBLEBUFFER, 1 }, { (SDL_GLattr)-1, 0 },
    };

    for (int32_t i = 0; attrs[i].attr != (SDL_GLattr)-1; i++) {
        if (SDL_GL_SetAttribute(attrs[i].attr, attrs[i].value) != 0) {
            LOG_ERROR(
                "Failed to set attribute %x: %s", attrs[i].attr,
                SDL_GetError());
        }
    }
}

static bool M_CreateGameWindow(void)
{
    int32_t result = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    if (result < 0) {
        Shell_ExitSystemFmt(
            "Error while calling SDL_Init: 0x%lx, %s", result, SDL_GetError());
        return false;
    }

    LOG_DEBUG(
        "%d,%d -> %dx%d", g_Config.window.x, g_Config.window.y,
        g_Config.window.width, g_Config.window.height);
    g_SDLWindow = SDL_CreateWindow(
        "TR2X", g_Config.window.x, g_Config.window.y, g_Config.window.width,
        g_Config.window.height,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    if (g_SDLWindow == nullptr) {
        Shell_ExitSystemFmt("Failed to create SDL window: %s", SDL_GetError());
        return false;
    }

    return true;
}

static void M_ShowHelp(void)
{
    puts("Currently available options:");
    puts("");
    puts("-g/--gold: launch The Golden Mask expansion pack.");
    puts("-l/--level <PATH>: launch a specific level file.");
    puts("-s/--save <NUM>: launch from a specific save slot (starts at 1).");
}

static bool M_ParseArgs(SHELL_ARGS *const out_args)
{
    const char **args = nullptr;
    int32_t arg_count = 0;
    Shell_GetCommandLine(&arg_count, &args);

    out_args->mod = M_MOD_OG;

    for (int32_t i = 0; i < arg_count; i++) {
        if (!strcmp(args[i], "-h") || !strcmp(args[i], "--help")) {
            M_ShowHelp();
            return false;
        }
        if (!strcmp(args[i], "-g") || !strcmp(args[i], "--gold")
            || !strcmp(args[i], "-gold")) {
            out_args->mod = M_MOD_GM;
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

static void M_LoadConfig(void)
{
    Config_Read();
    Config_SubscribeChanges(M_HandleConfigChange, nullptr);

    Sound_SetMasterVolume(g_Config.audio.sound_volume);
    Music_SetVolume(g_Config.audio.music_volume);
}

static void M_HandleConfigChange(const EVENT *const event, void *const data)
{
    if (m_IgnoreConfigChanges) {
        return;
    }

    const CONFIG *const old = &g_Config;
    const CONFIG *const new = &g_SavedConfig;

#define CHANGED(subject) (old->subject != new->subject)

    if (CHANGED(audio.sound_volume)) {
        Sound_SetMasterVolume(g_Config.audio.sound_volume);
    }
    if (CHANGED(audio.music_volume)) {
        Music_SetVolume(g_Config.audio.music_volume);
    }

    if (CHANGED(window.is_fullscreen) || CHANGED(window.is_maximized)
        || CHANGED(window.width) || CHANGED(window.height)
        || CHANGED(rendering.scaler) || CHANGED(rendering.sizer)
        || CHANGED(rendering.aspect_mode) || CHANGED(visuals.use_psx_fov)) {
        LOG_DEBUG("Change in settings detected");
        M_SyncToWindow();
        M_RefreshRendererViewport();
    }

    if (CHANGED(rendering.render_mode)) {
        Render_Reset(RENDER_RESET_ALL);
    } else if (
        CHANGED(rendering.enable_zbuffer)
        || CHANGED(rendering.enable_perspective_filter)
        || CHANGED(rendering.enable_wireframe)
        || CHANGED(rendering.wireframe_width)
        || CHANGED(rendering.texture_filter)
        || CHANGED(rendering.lighting_contrast)) {
        Render_Reset(RENDER_RESET_PARAMS);
    }

    if (CHANGED(visuals.fov) || CHANGED(visuals.use_psx_fov)) {
        if (Viewport_GetFOV(false) == -1) {
            Viewport_AlterFOV(-1);
        }
    }

    if (CHANGED(visuals.fog_start) || CHANGED(visuals.fog_end)
        || CHANGED(visuals.water_color.g) || CHANGED(visuals.water_color.b)
        || CHANGED(visuals.water_color.r)) {
        Output_ApplyLevelSettings();
    }

    if (CHANGED(rendering.aspect_mode)) {
        Output_ReloadBackgroundImage();
    }
}

// TODO: refactor the hell out of me
int32_t Shell_Main(void)
{
    if (!M_ParseArgs(&m_Args)) {
        return 0;
    }

    LOG_INFO("Game directory: %s", File_GetGameDirectory());

    if (m_Args.mod == M_MOD_GM) {
        Object_Get(O_MONK_3)->setup_func = Monk3_Setup;
        Object_Get(O_BEAR)->setup_func = Bear_Setup;
        Object_Get(O_WOLF)->setup_func = Wolf_Setup;
    } else {
        Object_Get(O_MONK_1)->setup_func = Monk1_Setup;
        Object_Get(O_SPIDER)->setup_func = Spider_Setup;
        Object_Get(O_BIG_SPIDER)->setup_func = BigSpider_Setup;
    }

    GameString_Init();
    EnumMap_Init();
    Config_Init();
    Text_Init();
    UI_Init();
    Console_Init();

    Input_Init();
    Sound_Init();
    Music_Init();

    M_LoadConfig();

    Clock_Init();

    if (!M_CreateGameWindow()) {
        Shell_ExitSystem("Failed to create game window");
        return 1;
    }

    Random_Seed();
    Output_CalculateWibbleTable();

    Shell_Start();
    Viewport_AlterFOV(-1);
    Viewport_Reset();
    Render_Reset(RENDER_RESET_PARAMS);

    GF_Init();
    GF_LoadFromFile(m_ModPaths[m_Args.mod].game_flow_path);
    GameStringTable_LoadFromFile(m_ModPaths[m_Args.mod].game_strings_path);
    GameStringTable_Apply(nullptr);

    GameBuf_Init();
    Level_Init();

    Savegame_Init();
    Savegame_InitCurrentInfo();
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
            const int32_t level_num = g_GameFlow.single_level >= 0
                ? g_GameFlow.single_level
                : gf_cmd.param;
            const GF_SEQUENCE_CONTEXT seq_ctx =
                gf_cmd.action == GF_SELECT_GAME ? GFSC_SELECT : GFSC_NORMAL;
            const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, level_num);
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
            if (g_GameFlow.title_level == nullptr) {
                gf_cmd = g_GameFlow.cmd_title;
                if (gf_cmd.action == GF_NOOP
                    || gf_cmd.action == GF_EXIT_TO_TITLE) {
                    Shell_ExitSystem("Title disabled & no replacement");
                    return 1;
                }
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
    GF_Shutdown();
    GameString_Shutdown();
    Console_Shutdown();
    Render_Shutdown();
    Text_Shutdown();
    UI_Shutdown();
    GameBuf_Shutdown();
    Config_Shutdown();
    EnumMap_Shutdown();
}

const char *Shell_GetConfigPath(void)
{
    return "cfg/TR2X.json5";
}

const char *Shell_GetGameFlowPath(void)
{
    return m_ModPaths[m_Args.mod].game_flow_path;
}

void Shell_Start(void)
{
    M_ConfigureOpenGL();
    Render_Init();
    M_SyncToWindow();

    SDL_ShowWindow(g_SDLWindow);
    SDL_RaiseWindow(g_SDLWindow);
    M_RefreshRendererViewport();
}

// TODO: try to call this function in a single place after introducing phases.
void Shell_ProcessEvents(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
            M_HandleQuit();
            break;

        case SDL_KEYDOWN: {
            M_HandleKeyDown(&event);
            break;
        }

        case SDL_KEYUP:
            M_HandleKeyUp(&event);
            break;

        case SDL_TEXTEDITING:
            UI_HandleTextEdit(event.text.text);
            break;

        case SDL_TEXTINPUT:
            UI_HandleTextEdit(event.text.text);
            break;

        case SDL_CONTROLLERDEVICEADDED:
        case SDL_JOYDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_JOYDEVICEREMOVED:
            Input_Discover();
            break;

        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_SHOWN:
                M_HandleWindowShown();
                break;

            case SDL_WINDOWEVENT_FOCUS_GAINED:
                M_HandleFocusGained();
                break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
                M_HandleFocusLost();
                break;

            case SDL_WINDOWEVENT_RESTORED:
                M_HandleWindowRestored();
                break;

            case SDL_WINDOWEVENT_MINIMIZED:
                M_HandleWindowMinimized();
                break;

            case SDL_WINDOWEVENT_MAXIMIZED:
                M_HandleWindowMaximized();
                break;

            case SDL_WINDOWEVENT_MOVED:
                M_HandleWindowMoved(event.window.data1, event.window.data2);
                break;

            case SDL_WINDOWEVENT_RESIZED:
                M_HandleWindowResized(event.window.data1, event.window.data2);
                break;
            }
            break;
        }
    }
}

SDL_Window *Shell_GetWindow(void)
{
    return g_SDLWindow;
}
