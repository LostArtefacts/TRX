#include "game/shell/common.h"

#include "decomp/decomp.h"
#include "decomp/savegame.h"
#include "game/clock.h"
#include "game/console/common.h"
#include "game/demo.h"
#include "game/fmv.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/music.h"
#include "game/output.h"
#include "game/phase.h"
#include "game/random.h"
#include "game/render/common.h"
#include "game/sound.h"
#include "game/text.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/enum_map.h>
#include <libtrx/game/game_buf.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/game/shell.h>
#include <libtrx/game/ui/common.h>
#include <libtrx/memory.h>

#include <SDL2/SDL.h>
#include <stdarg.h>
#include <stdio.h>

static Uint64 m_UpdateDebounce = 0;
static const char *m_CurrentGameFlowPath = "cfg/TR2X_gameflow.json5";
static const char *m_CurrentGameStringsPath = "cfg/TR2X_strings.json5";

static void M_SyncToWindow(void);
static void M_SyncFromWindow(void);
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
        if (x <= 0 || y <= 0) {
            x = (Shell_GetCurrentDisplayWidth() - width) / 2;
            y = (Shell_GetCurrentDisplayHeight() - height) / 2;
        }

        SDL_SetWindowFullscreen(g_SDLWindow, 0);
        SDL_SetWindowPosition(g_SDLWindow, x, y);
        SDL_SetWindowSize(g_SDLWindow, width, height);
        SDL_ShowCursor(SDL_ENABLE);
    }
}

static void M_SyncFromWindow(void)
{
    if (SDL_GetTicks() - m_UpdateDebounce < 1000) {
        // Setting the size programatically triggers resize events.
        // Additionally, SDL_GetWindowSize() is not guaranteed to return the
        // same values as passed to SDL_SetWindowSize(). In order to avoid
        // infinite loops where the window dimensions are continuously updated,
        // resize events are debounced.
        return;
    }

    const Uint32 window_flags = SDL_GetWindowFlags(g_SDLWindow);
    const bool is_maximized = window_flags & SDL_WINDOW_MAXIMIZED;

    int32_t width;
    int32_t height;
    SDL_GetWindowSize(g_SDLWindow, &width, &height);

    int32_t x;
    int32_t y;
    SDL_GetWindowPosition(g_SDLWindow, &x, &y);

    LOG_INFO("%dx%d+%d,%d (maximized: %d)", width, height, x, y, is_maximized);

    g_Config.window.is_maximized = is_maximized;
    if (!is_maximized && !g_Config.window.is_fullscreen) {
        g_Config.window.x = x;
        g_Config.window.y = y;
        g_Config.window.width = width;
        g_Config.window.height = height;
    }

    // Save the updated config, but ensure it was loaded first
    if (g_Config.loaded) {
        Config_Write();
    }

    M_RefreshRendererViewport();
}

static void M_RefreshRendererViewport(void)
{
    Viewport_Reset();
    UI_Events_Fire(&(EVENT) { .name = "canvas_resize" });
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
    M_SyncFromWindow();
}

static void M_HandleWindowMinimized(void)
{
    LOG_DEBUG("");
}

static void M_HandleWindowMaximized(void)
{
    M_SyncFromWindow();
}

static void M_HandleWindowMoved(const int32_t x, const int32_t y)
{
    M_SyncFromWindow();
}

static void M_HandleWindowResized(int32_t width, int32_t height)
{
    M_SyncFromWindow();
}

static void M_HandleKeyDown(const SDL_Event *const event)
{
    // NOTE: This normally would get handled by Input_Update,
    // but by the time Input_Update gets ran, we may already have lost
    // some keypresses if the player types really fast, so we need to
    // react sooner.
    if (!FMV_IsPlaying() && g_Config.gameplay.enable_console
        && !Console_IsOpened()
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

static void M_LoadConfig(void)
{
    Config_Read();
    Config_SubscribeChanges(M_HandleConfigChange, nullptr);

    Sound_SetMasterVolume(g_Config.audio.sound_volume);
    Music_SetVolume(g_Config.audio.music_volume);
}

static void M_HandleConfigChange(const EVENT *const event, void *const data)
{
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
        || CHANGED(window.x) || CHANGED(window.y) || CHANGED(window.width)
        || CHANGED(window.height) || CHANGED(rendering.scaler)
        || CHANGED(rendering.sizer) || CHANGED(rendering.aspect_mode)) {
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

    if (CHANGED(visuals.fov) || CHANGED(visuals.use_pcx_fov)) {
        if (Viewport_GetFOV(false) == -1) {
            Viewport_AlterFOV(-1);
        }
    }
}

// TODO: refactor the hell out of me
void Shell_Main(void)
{
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
        return;
    }

    Random_Seed();
    Output_CalculateWibbleTable();

    Shell_Start();
    Viewport_AlterFOV(-1);
    Viewport_Reset();
    Render_Reset(RENDER_RESET_PARAMS);

    GF_Init();
    GF_Load(m_CurrentGameFlowPath);
    GameStringTable_LoadFromFile(m_CurrentGameStringsPath);
    GameStringTable_Apply(nullptr);

    Savegame_Init();
    Savegame_InitCurrentInfo();
    S_FrontEndCheck();

    GameBuf_Init();

    GF_COMMAND gf_cmd = GF_DoFrontendSequence();
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
            S_LoadGame(gf_cmd.param);
            const GF_LEVEL *const level =
                GF_GetLevel(GFLT_MAIN, g_SaveGame.current_level);
            if (level != nullptr) {
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
                    return;
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
    return m_CurrentGameFlowPath;
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

bool Shell_IsFullscreen(void)
{
    const Uint32 flags = SDL_GetWindowFlags(g_SDLWindow);
    return (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
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
