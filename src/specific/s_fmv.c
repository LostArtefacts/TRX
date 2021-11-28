#include "specific/s_fmv.h"

#include "config.h"
#include "filesystem.h"
#include "game/clock.h"
#include "game/gamebuf.h"
#include "game/input.h"
#include "game/screen.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "log.h"
#include "memory.h"
#include "specific/s_hwr.h"
#include "specific/s_shell.h"

#include <ddraw.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

HMODULE m_PlayerModule = NULL;

int32_t (*Movie_GetCurrentFrame)(void *) = NULL;
int32_t (*Movie_GetSoundChannels)(void *) = NULL;
int32_t (*Movie_GetSoundPrecision)(void *) = NULL;
int32_t (*Movie_GetSoundRate)(void *) = NULL;
int32_t (*Movie_GetTotalFrames)(void *) = NULL;
int32_t (*Movie_GetXSize)(void *) = NULL;
int32_t (*Movie_GetYSize)(void *) = NULL;
int32_t (*Player_GetDSErrorCode)() = NULL;
int32_t (*Player_InitMovie)(
    void *, uint32_t, uint32_t, const char *, uint32_t) = NULL;
int32_t (*Player_InitMoviePlayback)(HWND, void *, void *) = NULL;
int32_t (*Player_InitPlaybackMode)(void *, void *, uint32_t, uint32_t) = NULL;
int32_t (*Player_InitSound)(
    void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
    int32_t) = NULL;
int32_t (*Player_InitSoundSystem)(HWND) = NULL;
int32_t (*Player_InitVideo)(
    void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
    int32_t, int32_t, int32_t, int32_t, int32_t) = NULL;
int32_t (*Player_MapVideo)(void *, int32_t) = NULL;
int32_t (*Player_PassInDirectDrawObject)(LPDIRECTDRAW) = NULL;
int32_t (*Player_PlayFrame)(
    void *, void *, void *, uint32_t, void *, uint32_t, uint32_t,
    uint32_t) = NULL;
int32_t (*Player_ReturnPlaybackMode)() = NULL;
int32_t (*Player_ShutDownMovie)(void *) = NULL;
int32_t (*Player_ShutDownSound)(void *) = NULL;
int32_t (*Player_ShutDownVideo)(void *) = NULL;
int32_t (*Player_StartTimer)(void *) = NULL;

bool S_FMV_Init()
{
    m_PlayerModule = LoadLibraryA("winplay");
    if (!m_PlayerModule) {
        LOG_ERROR("cannot find winplay.dll");
        goto fail;
    }

    Movie_GetCurrentFrame = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Movie_GetCurrentFrame");
    if (!Movie_GetCurrentFrame) {
        LOG_ERROR("cannot find Movie_GetCurrentFrame");
        goto fail;
    }

    Movie_GetSoundChannels = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Movie_GetSoundChannels");
    if (!Movie_GetSoundChannels) {
        LOG_ERROR("cannot find Movie_GetSoundChannels");
        goto fail;
    }

    Movie_GetSoundPrecision = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Movie_GetSoundPrecision");
    if (!Movie_GetSoundPrecision) {
        LOG_ERROR("cannot find Movie_GetSoundPrecision");
        goto fail;
    }

    Movie_GetSoundRate = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Movie_GetSoundRate");
    if (!Movie_GetSoundRate) {
        LOG_ERROR("cannot find Movie_GetSoundRate");
        goto fail;
    }

    Movie_GetTotalFrames = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Movie_GetTotalFrames");
    if (!Movie_GetTotalFrames) {
        LOG_ERROR("cannot find Movie_GetTotalFrames");
        goto fail;
    }

    Movie_GetXSize =
        (int32_t(*)(void *))GetProcAddress(m_PlayerModule, "Movie_GetXSize");
    if (!Movie_GetXSize) {
        LOG_ERROR("cannot find Movie_GetXSize");
        goto fail;
    }

    Movie_GetYSize =
        (int32_t(*)(void *))GetProcAddress(m_PlayerModule, "Movie_GetYSize");
    if (!Movie_GetYSize) {
        LOG_ERROR("cannot find Movie_GetYSize");
        goto fail;
    }

    Player_GetDSErrorCode =
        (int32_t(*)())GetProcAddress(m_PlayerModule, "Player_GetDSErrorCode");
    if (!Player_GetDSErrorCode) {
        LOG_ERROR("cannot find Player_GetDSErrorCode");
        goto fail;
    }

    Player_InitMovie =
        (int32_t(*)(void *, uint32_t, uint32_t, const char *, uint32_t))
            GetProcAddress(m_PlayerModule, "Player_InitMovie");
    if (!Player_InitMovie) {
        LOG_ERROR("cannot find Player_InitMovie");
        goto fail;
    }

    Player_InitMoviePlayback = (int32_t(*)(HWND, void *, void *))GetProcAddress(
        m_PlayerModule, "Player_InitMoviePlayback");
    if (!Player_InitMoviePlayback) {
        LOG_ERROR("cannot find Player_InitMoviePlayback");
        goto fail;
    }

    Player_InitPlaybackMode =
        (int32_t(*)(void *, void *, uint32_t, uint32_t))GetProcAddress(
            m_PlayerModule, "Player_InitPlaybackMode");
    if (!Player_InitPlaybackMode) {
        LOG_ERROR("cannot find Player_InitPlaybackMode");
        goto fail;
    }

    Player_InitSound = (int32_t(*)(
        void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
        int32_t))GetProcAddress(m_PlayerModule, "Player_InitSound");
    if (!Player_InitSound) {
        LOG_ERROR("cannot find Player_InitSound");
        goto fail;
    }

    Player_InitSoundSystem = (int32_t(*)(HWND))GetProcAddress(
        m_PlayerModule, "Player_InitSoundSystem");
    if (!Player_InitSoundSystem) {
        LOG_ERROR("cannot find Player_InitSoundSystem");
        goto fail;
    }

    Player_InitVideo = (int32_t(*)(
        void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
        int32_t, int32_t, int32_t, int32_t,
        int32_t))GetProcAddress(m_PlayerModule, "Player_InitVideo");
    if (!Player_InitVideo) {
        LOG_ERROR("cannot find Player_InitVideo");
        goto fail;
    }

    Player_MapVideo = (int32_t(*)(void *, int32_t))GetProcAddress(
        m_PlayerModule, "Player_MapVideo");
    if (!Player_MapVideo) {
        LOG_ERROR("cannot find Player_MapVideo");
        goto fail;
    }

    Player_PassInDirectDrawObject = (int32_t(*)(LPDIRECTDRAW))GetProcAddress(
        m_PlayerModule, "Player_PassInDirectDrawObject");
    if (!Player_PassInDirectDrawObject) {
        LOG_ERROR("cannot find Player_PassInDirectDrawObject");
        goto fail;
    }

    Player_PlayFrame = (int32_t(*)(
        void *, void *, void *, uint32_t, void *, uint32_t, uint32_t,
        uint32_t))GetProcAddress(m_PlayerModule, "Player_PlayFrame");
    if (!Player_PlayFrame) {
        LOG_ERROR("cannot find Player_PlayFrame");
        goto fail;
    }

    Player_ReturnPlaybackMode = (int32_t(*)())GetProcAddress(
        m_PlayerModule, "Player_ReturnPlaybackMode");
    if (!Player_ReturnPlaybackMode) {
        LOG_ERROR("cannot find Player_ReturnPlaybackMode");
        goto fail;
    }

    Player_ShutDownMovie = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Player_ShutDownMovie");
    if (!Player_ShutDownMovie) {
        LOG_ERROR("cannot find Player_ShutDownMovie");
        goto fail;
    }

    Player_ShutDownSound = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Player_ShutDownSound");
    if (!Player_ShutDownSound) {
        LOG_ERROR("cannot find Player_ShutDownSound");
        goto fail;
    }

    Player_ShutDownVideo = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Player_ShutDownVideo");
    if (!Player_ShutDownVideo) {
        LOG_ERROR("cannot find Player_ShutDownVideo");
        goto fail;
    }

    Player_StartTimer =
        (int32_t(*)(void *))GetProcAddress(m_PlayerModule, "Player_StartTimer");

    if (!Player_StartTimer) {
        LOG_ERROR("cannot find Player_StartTimer");
        goto fail;
    }
    if (Player_PassInDirectDrawObject(g_DDraw)) {
        LOG_ERROR("ERROR: Cannot initialise FMV player videosystem");
        goto fail;
    }
    if (Player_InitSoundSystem(g_TombHWND)) {
        Player_GetDSErrorCode();
        LOG_ERROR("ERROR: Cannot prepare FMV player soundsystem");
        goto fail;
    }

    return true;

fail:
    if (m_PlayerModule) {
        FreeLibrary(m_PlayerModule);
        m_PlayerModule = NULL;
    }
    return false;
}

void S_FMV_Play(const char *file_path)
{
    if (g_Config.disable_fmv || !m_PlayerModule) {
        return;
    }

    char *full_path = NULL;
    File_GetFullPath(file_path, &full_path);

    GameBuf_Shutdown();
    Screen_SetResolution(2);
    HWR_FMVInit();

    void *movie_context = NULL;
    void *fmv_context = NULL;
    void *sound_context = NULL;
    if (Player_InitMovie(&movie_context, 0, 0, full_path, 0x100000)) {
        LOG_ERROR("cannot load video");
        goto cleanup;
    }

    int32_t width = Movie_GetXSize(movie_context);
    int32_t height = Movie_GetYSize(movie_context);
    int32_t x = 0;
    int32_t y = 0;
    int32_t flags = 13;
    y = (480 - 2 * height) / 2;

    if (Player_InitVideo(
            &fmv_context, movie_context, width, height, x, y, 0, 0, 640, 480, 0,
            1, flags)) {
        LOG_ERROR("cannot init video");
        goto cleanup;
    }

    if (Player_InitPlaybackMode(g_TombHWND, fmv_context, 1, 0)) { //
        LOG_ERROR("cannot init playback mode");
        goto cleanup;
    }

    if (g_SoundIsActive) {
        int32_t precision = Movie_GetSoundPrecision(movie_context);
        int32_t rate = Movie_GetSoundRate(movie_context);
        int32_t channels = Movie_GetSoundChannels(movie_context);
        if (Player_InitSound(
                &sound_context, 44100, 1, 1, 22050, channels, rate, precision,
                2)) {
            LOG_ERROR("cannot init sound");
            goto cleanup;
        }
    }

    if (Player_InitMoviePlayback(movie_context, fmv_context, sound_context)) {
        LOG_ERROR("cannot init movie playback");
        goto cleanup;
    }

    if (Player_MapVideo(fmv_context, 0)) {
        LOG_ERROR("cannot map video");
        goto cleanup;
    }

    bool keypress = false;
    int32_t total_frames = Movie_GetTotalFrames(movie_context);
    if (Player_StartTimer(movie_context)) {
        LOG_ERROR("cannot start timer");
        goto cleanup;
    }

    while (Movie_GetCurrentFrame(movie_context) < total_frames - 2) {
        if (Player_PlayFrame(
                movie_context, fmv_context, sound_context, 0, 0, 0, 0, 0)) {
            break;
        }
        Input_Update();
        S_Shell_SpinMessageLoop();
        Clock_Sync();

        if (g_Input.deselect) {
            keypress = true;
        } else if (keypress && !g_Input.deselect) {
            break;
        }
    }

    Player_ShutDownSound(&sound_context);
    Player_ShutDownVideo(&fmv_context);
    Player_ShutDownMovie(&movie_context);

cleanup:
    if (full_path) {
        Memory_Free(full_path);
    }

    Player_ReturnPlaybackMode();
    GameBuf_Init();
    HWR_FMVDone();

    Screen_RestoreResolution();
}
