#include "specific/s_shell.h"

#include "config.h"
#include "game/gamebuf.h"
#include "game/input.h"
#include "game/music.h"
#include "game/random.h"
#include "game/shell.h"
#include "global/vars_platform.h"
#include "log.h"
#include "memory.h"
#include "specific/s_hwr.h"

#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <ddraw.h>
#include <time.h>

static int m_ArgCount = 0;
static char **m_ArgStrings = NULL;
static bool m_Fullscreen = true;
static SDL_Window *m_Window = NULL;
static HMODULE m_DDraw = NULL;
static HRESULT (*m_DirectDrawCreate)(GUID *, LPDIRECTDRAW *, IUnknown *) = NULL;

static bool S_Shell_InitDirectDraw();
static void S_Shell_TerminateGame(int exit_code);
static void S_Shell_ShowFatalError(const char *message);
static void S_Shell_PostWindowResize();
static void S_Shell_ToggleFullscreen();

void S_Shell_SeedRandom()
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);
    Random_SeedControl(tptr->tm_sec + 57 * tptr->tm_min + 3543 * tptr->tm_hour);
    Random_SeedDraw(tptr->tm_sec + 43 * tptr->tm_min + 3477 * tptr->tm_hour);
}

static bool S_Shell_InitDirectDraw()
{
    if (!g_GLRage) {
        g_GLRage = LoadLibrary("glrage.dll");
    }

    m_DDraw = g_GLRage;
    if (!m_DDraw) {
        S_Shell_ShowFatalError("Cannot find glrage.dll");
        return false;
    }

    m_DirectDrawCreate =
        (HRESULT(*)(GUID *, LPDIRECTDRAW *, IUnknown *))GetProcAddress(
            m_DDraw, "DirectDrawCreate");
    if (!m_DirectDrawCreate) {
        S_Shell_ShowFatalError("Cannot find DirectDrawCreate");
        return false;
    }

    if (m_DirectDrawCreate(NULL, &g_DDraw, NULL)) {
        S_Shell_ShowFatalError("DirectDraw could not be started");
        return false;
    }

    if (S_ATI_Init()) {
        S_Shell_ShowFatalError("ATI3DCIF could not be started");
        return false;
    }

    return true;
}

static void S_Shell_TerminateGame(int exit_code)
{
    if (g_DDraw) {
        HWR_ReleaseSurfaces();
        IDirectDraw_FlipToGDISurface(g_DDraw);
        IDirectDraw_FlipToGDISurface(g_DDraw);
        IDirectDraw_RestoreDisplayMode(g_DDraw);
        IDirectDraw_SetCooperativeLevel(g_DDraw, g_TombHWND, 8);
        IDirectDraw_Release(g_DDraw);
        g_DDraw = NULL;
    }

    S_ATI_Shutdown();

    if (m_Window) {
        SDL_DestroyWindow(m_Window);
    }
    SDL_Quit();
    exit(exit_code);
}

static void S_Shell_ShowFatalError(const char *message)
{
    LOG_ERROR("%s", message);
    MessageBoxA(
        0, message, "Tomb Raider Error", MB_SETFOREGROUND | MB_ICONEXCLAMATION);
    S_Shell_TerminateGame(1);
}

static void S_Shell_PostWindowResize()
{
    int width;
    int height;
    SDL_GetWindowSize(m_Window, &width, &height);
    HWR_SetViewport(width, height);
}

static void S_Shell_ToggleFullscreen()
{
    m_Fullscreen = !m_Fullscreen;
    HWR_SetFullscreen(m_Fullscreen);
    SDL_SetWindowFullscreen(
        m_Window, m_Fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    SDL_ShowCursor(m_Fullscreen ? SDL_DISABLE : SDL_ENABLE);
    S_Shell_PostWindowResize();
}

void S_Shell_SpinMessageLoop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
            S_Shell_TerminateGame(0);
            break;

        case SDL_KEYDOWN: {
            const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);
            if (keyboard_state[SDL_SCANCODE_LALT]
                && keyboard_state[SDL_SCANCODE_RETURN]) {
                S_Shell_ToggleFullscreen();
            }
            break;
        }

        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                Music_SetVolume(g_Config.music_volume);
                break;

            case SDL_WINDOWEVENT_FOCUS_LOST:
                Music_SetVolume(0);
                break;

            case SDL_WINDOWEVENT_RESIZED: {
                HWR_SetViewport(event.window.data1, event.window.data2);
                break;
            }
            }
            break;
        }
    }
}

int main(int argc, char **argv)
{
    Log_Init();

    m_ArgCount = argc;
    m_ArgStrings = argv;

    if (SDL_Init(SDL_INIT_EVENTS) < 0) {
        S_Shell_ExitSystemFmt("Cannot initialize SDL: %s", SDL_GetError());
        return 1;
    }

    m_Window = SDL_CreateWindow(
        "Tomb1Main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
            | SDL_WINDOW_RESIZABLE);
    if (!m_Window) {
        S_Shell_ShowFatalError("System Error: cannot create window");
        return 1;
    }

    S_Shell_PostWindowResize();

    SDL_ShowCursor(SDL_DISABLE);

    SDL_SysWMinfo wm_info;
    SDL_VERSION(&wm_info.version);
    SDL_GetWindowWMInfo(m_Window, &wm_info);
    g_TombModule = wm_info.info.win.hinstance;
    g_TombHWND = wm_info.info.win.window;

    if (!g_TombHWND) {
        S_Shell_ShowFatalError("System Error: cannot create window");
        return 1;
    }

    if (!S_Shell_InitDirectDraw()) {
        S_Shell_TerminateGame(1);
        return 1;
    }

    Shell_Main();

    S_Shell_TerminateGame(0);
    return 0;
}

void S_Shell_ExitSystem(const char *message)
{
    while (g_Input.select) {
        Input_Update();
    }
    GameBuf_Shutdown();
    HWR_ShutdownHardware();
    S_Shell_ShowFatalError(message);
}

void S_Shell_ExitSystemFmt(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    char message[150];
    vsnprintf(message, 150, fmt, va);
    va_end(va);
    S_Shell_ExitSystem(message);
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
