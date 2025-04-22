#include "debug.h"
#include "game/shell.h"
#include "log.h"
#include "memory.h"

#ifdef _WIN32
    #include <objbase.h>
    #include <windows.h>
#endif

#include <libavcodec/version.h>
#include <stdio.h>

static bool m_IsExiting = false;

static void M_SetupHiDPI(void);
static void M_SetupLibAV(void);
static void M_SetupSDL(void);
static void M_ShowFatalError(const char *message);

static void M_SetupHiDPI(void)
{
#ifdef _WIN32
    // Enable HiDPI mode in Windows to detect DPI scaling
    typedef enum {
        PROCESS_DPI_UNAWARE = 0,
        PROCESS_SYSTEM_DPI_AWARE = 1,
        PROCESS_PER_MONITOR_DPI_AWARE = 2
    } PROCESS_DPI_AWARENESS;

    // Windows 8.1 and later
    void *const shcore_dll = SDL_LoadObject("SHCORE.DLL");
    if (shcore_dll == nullptr) {
        return;
    }

    #pragma GCC diagnostic ignored "-Wpedantic"
    HRESULT(WINAPI * SetProcessDpiAwareness)
    (PROCESS_DPI_AWARENESS) =
        (HRESULT(WINAPI *)(PROCESS_DPI_AWARENESS))SDL_LoadFunction(
            shcore_dll, "SetProcessDpiAwareness");
    #pragma GCC diagnostic pop
    if (SetProcessDpiAwareness == nullptr) {
        return;
    }
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
#endif
}

static void M_SetupLibAV(void)
{
#ifdef _WIN32
    // necessary for SDL_OpenAudioDevice to work with WASAPI
    // https://www.mail-archive.com/ffmpeg-trac@avcodec.org/msg43300.html
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

#if LIBAVCODEC_VERSION_MAJOR <= 57
    av_register_all();
#endif
}

static void M_SetupSDL(void)
{
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0) {
        Shell_ExitSystemFmt("Cannot initialize SDL: %s", SDL_GetError());
    }
}

static void M_SetupGL(void)
{
    // Setup minimum properties of GL context
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

static void M_ShowFatalError(const char *const message)
{
    LOG_ERROR("%s", message);
    SDL_Window *const window = Shell_GetWindow();
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR, "Tomb Raider Error", message, window);
    Shell_Terminate(1);
}

void Shell_Setup(void)
{
    M_SetupHiDPI();
    M_SetupLibAV();
    M_SetupSDL();
    M_SetupGL();
}

void Shell_Terminate(int32_t exit_code)
{
    Shell_Shutdown();

    SDL_Window *const window = Shell_GetWindow();
    if (window != nullptr) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    exit(exit_code);
}

void Shell_ExitSystem(const char *message)
{
    M_ShowFatalError(message);
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

SHELL_SIZE Shell_GetWindowSize(void)
{
    SDL_Window *const window = Shell_GetWindow();
    SHELL_SIZE result = { .w = -1, .h = -1 };
    if (window != nullptr) {
        SDL_GetWindowSize(window, &result.w, &result.h);
    }
    return result;
}

SHELL_SIZE Shell_GetCurrentSize(void)
{
    return Shell_IsFullscreen() ? Shell_GetCurrentDisplaySize()
                                : Shell_GetWindowSize();
}

SHELL_SIZE Shell_GetCurrentDisplaySize(void)
{
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    return (SHELL_SIZE) { .w = dm.w, .h = dm.h };
}

void Shell_ScheduleExit(void)
{
    m_IsExiting = true;
}

bool Shell_IsExiting(void)
{
    return m_IsExiting;
}
