#include <trx/av/audio.h>
#include <trx/config.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/debug.h>
#include <trx/game/replay/test_replay.h>
#include <trx/game/shell.h>

#ifdef _WIN32
    #include <objbase.h>
    #include <windows.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_messagebox.h>
#include <libavcodec/version.h>
#include <libavutil/log.h>
#include <stdio.h>

// Limits the fatal error dialog width, in characters.
#define M_DIALOG_COLUMNS 80

static bool m_IsExiting = false;
static bool m_IsFocused = true;

// SDL_ShowSimpleMessageBox blocks until the dialog is dismissed, which never
// happens in a batch run, so such a run hangs instead of reporting the error
// and stopping.
static bool M_IsInteractive(void)
{
    const SHELL_ARGS *const args = Shell_GetArgs();
    if (args == nullptr) {
        return true;
    }
    return !args->headless && !args->startup.dump_lua_api;
}

static void M_ShowFatalError(
    const char *const log_message, const char *const dialog_message)
{
    LOG_ERROR("%s", log_message);
    Log_Flush();
    if (M_IsInteractive()) {
        // The dialog is placed over its parent window. Until the game window is
        // shown, it is still hidden at the position the config named, so the
        // dialog is better off centered on the screen instead.
        SDL_Window *window = Shell_GetWindow();
        if (window != nullptr
            && (SDL_GetWindowFlags(window) & SDL_WINDOW_SHOWN) == 0) {
            window = nullptr;
        }
        char *const wrapped = String_Wrap(dialog_message, M_DIALOG_COLUMNS);
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR, "Tomb Raider Error",
            wrapped != nullptr ? wrapped : dialog_message, window);
        Memory_Free(wrapped);
    }
    Shell_Terminate(1);
}

const char *Shell_GetConfigDir(void)
{
    return GamePath_Get(GAME_PATH_CONFIG_DIR);
}

const char *Shell_GetCacheDir(void)
{
    return GamePath_Get(GAME_PATH_CACHE_DIR);
}

void Shell_Terminate(int32_t exit_code)
{
    Shell_Shutdown();

    SDL_Window *const window = Shell_GetWindow();
    if (window != nullptr) {
        SDL_DestroyWindow(window);
    }
    if (Audio_ShouldSkipSDLQuitAudio()) {
        const Uint32 inited = SDL_WasInit(0);
        const Uint32 quit_flags = inited & ~SDL_INIT_AUDIO;
        if (quit_flags != 0) {
            SDL_QuitSubSystem(quit_flags);
        }
    } else {
        SDL_Quit();
    }
    exit(exit_code);
}

void Shell_ExitSystem(const char *message)
{
    M_ShowFatalError(message, message);
    Shell_Shutdown();
}

void Shell_ExitSystemEx(
    const char *const log_message, const char *const dialog_message)
{
    M_ShowFatalError(log_message, dialog_message);
    Shell_Shutdown();
}

void Shell_ExitSystemFmt(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int32_t size = vsnprintf(nullptr, 0, fmt, va) + 1;
    char *message = Memory_Alloc(size);
    va_end(va);

    va_start(va, fmt);
    vsnprintf(message, size, fmt, va);
    va_end(va);

    Shell_ExitSystem(message);

    Memory_FreePointer(&message);
}

bool Shell_IsFullscreen(void)
{
    SDL_Window *const window = Shell_GetWindow();
    ASSERT(window != nullptr);
    const Uint32 flags = SDL_GetWindowFlags(window);
    return (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
}

SHELL_SIZE Shell_GetCurrentSize(void)
{
    return Shell_IsFullscreen() ? Shell_GetCurrentDisplaySize()
                                : Shell_GetWindowSize();
}

SHELL_SIZE Shell_GetDefaultSize(void)
{
    return (SHELL_SIZE) { SHELL_HEADLESS_WIDTH, SHELL_HEADLESS_HEIGHT };
}

SHELL_SIZE Shell_GetWindowSize(void)
{
    if (Shell_GetArgs()->headless) {
        return Shell_GetDefaultSize();
    }
    SDL_Window *const window = Shell_GetWindow();
    SHELL_SIZE result = { .w = -1, .h = -1 };
    if (window != nullptr) {
        SDL_GetWindowSize(window, &result.w, &result.h);
    }
    return result;
}

SHELL_SIZE Shell_GetCurrentDisplaySize(void)
{
    if (Shell_GetArgs()->headless) {
        return Shell_GetDefaultSize();
    }
    int32_t display_idx = 0;
    SDL_Window *const window = Shell_GetWindow();
    if (window != nullptr) {
        display_idx = SDL_GetWindowDisplayIndex(window);
    }
    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(display_idx, &dm) == 0) {
        return (SHELL_SIZE) { .w = dm.w, .h = dm.h };
    }
    return (SHELL_SIZE) { .w = -1, .h = -1 };
}

void Shell_ScheduleExit(void)
{
    m_IsExiting = true;
}

bool Shell_IsExiting(void)
{
    return m_IsExiting;
}

void Shell_SetIsFocused(const bool is_focused)
{
    m_IsFocused = is_focused;
}

bool Shell_IsFocused(void)
{
    return m_IsFocused;
}

bool Shell_ShouldPauseForFocusLoss(void)
{
    // A recording's events are read before anything asks this, so pausing
    // would fire them into a game that is not running.
    if (TestReplay_IsOpened()) {
        return false;
    }
    return g_Config.gameplay.pause_on_focus_lost && !Shell_IsFocused();
}
