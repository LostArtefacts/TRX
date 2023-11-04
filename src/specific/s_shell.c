#include "specific/s_shell.h"

#include "config.h"
#include "game/console.h"
#include "game/input.h"
#include "game/music.h"
#include "game/output.h"
#include "game/random.h"
#include "game/shell.h"
#include "game/sound.h"
#include "log.h"
#include "memory.h"

#define SDL_MAIN_HANDLED

#ifdef _WIN32
    #include <objbase.h>
    #include <windows.h>
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_messagebox.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_video.h>
#include <libavcodec/version.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int m_ArgCount = 0;
static char **m_ArgStrings = NULL;
static SDL_Window *m_Window = NULL;

static void S_Shell_SeedRandom(void);
static void S_Shell_SetWindowPos(int32_t x, int32_t y, bool update);
static void S_Shell_SetWindowSize(int32_t width, int32_t height, bool update);
static void S_Shell_SetWindowMaximized(bool is_enabled, bool update);
static void S_Shell_SetFullscreen(bool is_enabled, bool update);

static void S_Shell_SeedRandom(void)
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);
    Random_SeedControl(tptr->tm_sec + 57 * tptr->tm_min + 3543 * tptr->tm_hour);
    Random_SeedDraw(tptr->tm_sec + 43 * tptr->tm_min + 3477 * tptr->tm_hour);
}

static void S_Shell_SetWindowPos(int32_t x, int32_t y, bool update)
{
    if (x <= 0 || y <= 0) {
        return;
    }

    // only save window position if it's in windowed state.
    if (!g_Config.rendering.enable_fullscreen
        && !g_Config.rendering.enable_maximized) {
        g_Config.rendering.window_x = x;
        g_Config.rendering.window_y = y;
    }

    if (update) {
        SDL_SetWindowPosition(m_Window, x, y);
    }
}

static void S_Shell_SetWindowSize(int32_t width, int32_t height, bool update)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    // only save window size if it's in windowed state.
    if (!g_Config.rendering.enable_fullscreen
        && !g_Config.rendering.enable_maximized) {
        g_Config.rendering.window_width = width;
        g_Config.rendering.window_height = height;
    }

    Output_SetWindowSize(width, height);

    if (update) {
        SDL_SetWindowSize(m_Window, width, height);
    }
}

static void S_Shell_SetWindowMaximized(bool is_enabled, bool update)
{
    g_Config.rendering.enable_maximized = is_enabled;

    if (update && is_enabled) {
        SDL_MaximizeWindow(m_Window);
    }
}

static void S_Shell_SetFullscreen(bool is_enabled, bool update)
{
    g_Config.rendering.enable_fullscreen = is_enabled;

    if (update) {
        SDL_SetWindowFullscreen(
            m_Window, is_enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_ShowCursor(is_enabled ? SDL_DISABLE : SDL_ENABLE);
    }
}

void S_Shell_ToggleFullscreen(void)
{
    S_Shell_SetFullscreen(!g_Config.rendering.enable_fullscreen, true);

    // save the updated config, but ensure it was loaded first
    if (g_Config.loaded) {
        Config_Write();
    }
}

void S_Shell_HandleWindowResize(void)
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

    S_Shell_SetWindowMaximized(is_maximized, false);
    S_Shell_SetWindowPos(x, y, false);
    S_Shell_SetWindowSize(width, height, false);

    // save the updated config, but ensure it was loaded first
    if (g_Config.loaded) {
        Config_Write();
    }
}

void S_Shell_Init(void)
{
    S_Shell_SeedRandom();

    S_Shell_SetFullscreen(g_Config.rendering.enable_fullscreen, true);
    S_Shell_SetWindowPos(
        g_Config.rendering.window_x, g_Config.rendering.window_y, true);
    S_Shell_SetWindowSize(
        g_Config.rendering.window_width, g_Config.rendering.window_height,
        true);
    S_Shell_SetWindowMaximized(g_Config.rendering.enable_maximized, true);
}

void S_Shell_ShowFatalError(const char *message)
{
    LOG_ERROR("%s", message);
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR, "Tomb Raider Error", message, m_Window);
    S_Shell_TerminateGame(1);
}

void S_Shell_TerminateGame(int exit_code)
{
    Shell_Shutdown();
    if (m_Window) {
        SDL_DestroyWindow(m_Window);
    }
    SDL_Quit();
    exit(exit_code);
}

void S_Shell_SpinMessageLoop(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
            S_Shell_TerminateGame(0);
            break;

        case SDL_KEYUP:
            if (event.key.keysym.sym == SDLK_PRINTSCREEN) {
                Shell_MakeScreenshot();
                break;
            }

            if (event.key.keysym.sym == SDLK_RETURN
                && event.key.keysym.mod & KMOD_LALT) {
                S_Shell_ToggleFullscreen();
                break;
            }

            break;

        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                Music_SetVolume(g_Config.music_volume);
                Sound_SetMasterVolume(g_Config.sound_volume);
                break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
                Music_SetVolume(0);
                Sound_SetMasterVolume(0);
                break;

            case SDL_WINDOWEVENT_MOVED:
            case SDL_WINDOWEVENT_RESIZED:
                S_Shell_HandleWindowResize();
                break;
            }
            break;

        case SDL_KEYDOWN:
            Console_HandleKeyDown(event);
            break;

        case SDL_TEXTEDITING:
            Console_HandleTextEdit(event);
            break;

        case SDL_TEXTINPUT:
            Console_HandleTextInput(event);
            break;

        case SDL_CONTROLLERDEVICEADDED:
        case SDL_JOYDEVICEADDED:
            Input_InitController();
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_JOYDEVICEREMOVED:
            Input_ShutdownController();
            break;
        }
    }
}

int main(int argc, char **argv)
{
    Log_Init();

#ifdef _WIN32
    // Enable HiDPI mode in Windows to detect DPI scaling
    typedef enum PROCESS_DPI_AWARENESS {
        PROCESS_DPI_UNAWARE = 0,
        PROCESS_SYSTEM_DPI_AWARE = 1,
        PROCESS_PER_MONITOR_DPI_AWARE = 2
    } PROCESS_DPI_AWARENESS;

    HRESULT(WINAPI * SetProcessDpiAwareness)
    (PROCESS_DPI_AWARENESS dpiAwareness); // Windows 8.1 and later
    void *shcore_dll = SDL_LoadObject("SHCORE.DLL");
    if (shcore_dll) {
        SetProcessDpiAwareness =
            (HRESULT(WINAPI *)(PROCESS_DPI_AWARENESS))SDL_LoadFunction(
                shcore_dll, "SetProcessDpiAwareness");
        if (SetProcessDpiAwareness) {
            SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
        }
    }

    // necessary for SDL_OpenAudioDevice to work with WASAPI
    // https://www.mail-archive.com/ffmpeg-trac@avcodec.org/msg43300.html
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif

#if LIBAVCODEC_VERSION_MAJOR <= 57
    av_register_all();
#endif

    m_ArgCount = argc;
    m_ArgStrings = argv;

    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0) {
        char buf[256];
        sprintf(buf, "Cannot initialize SDL: %s", SDL_GetError());
        S_Shell_ShowFatalError(buf);
        return 1;
    }

    // Setup minimum properties of GL context
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    m_Window = SDL_CreateWindow(
        "TR1X", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE
            | SDL_WINDOW_OPENGL);

    if (!m_Window) {
        S_Shell_ShowFatalError("System Error: cannot create window");
        return 1;
    }

    S_Shell_HandleWindowResize();

    Shell_Main();

    S_Shell_TerminateGame(0);
    return 0;
}

bool S_Shell_GetCommandLine(int *arg_count, char ***args)
{
    *arg_count = m_ArgCount;
    *args = Memory_Alloc(m_ArgCount * sizeof(char *));
    for (int i = 0; i < m_ArgCount; i++) {
        (*args)[i] = Memory_Alloc(strlen(m_ArgStrings[i]) + 1);
        strcpy((*args)[i], m_ArgStrings[i]);
    }
    return true;
}

void *S_Shell_GetWindowHandle(void)
{
    return (void *)m_Window;
}

int S_Shell_GetCurrentDisplayWidth(void)
{
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    return dm.w;
}

int S_Shell_GetCurrentDisplayHeight(void)
{
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    return dm.h;
}
