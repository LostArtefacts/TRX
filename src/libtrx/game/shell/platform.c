#include "game/shell/platform.h"

#ifdef _WIN32
    #include <objbase.h>
    #include <windows.h>
#endif

#include <SDL2/SDL.h>
#include <libavcodec/version.h>
#include <libavutil/log.h>

void Shell_SetupHiDPI(void)
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

void Shell_SetupLibAV(void)
{
#ifdef _WIN32
    // necessary for SDL_OpenAudioDevice to work with WASAPI
    // https://www.mail-archive.com/ffmpeg-trac@avcodec.org/msg43300.html
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

#if LIBAVCODEC_VERSION_MAJOR <= 57
    av_register_all();
#endif

    av_log_set_level(AV_LOG_ERROR);
}
