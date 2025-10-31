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
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "0");
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
