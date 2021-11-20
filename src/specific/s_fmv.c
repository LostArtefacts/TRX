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

HMODULE m_PlayerModule;

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

void S_FMV_Init()
{
    m_PlayerModule = LoadLibraryA("winplay");
    if (!m_PlayerModule) {
        S_Shell_ExitSystem("cannot find winplay.dll");
    }

    Movie_GetCurrentFrame = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Movie_GetCurrentFrame");
    if (!Movie_GetCurrentFrame) {
        S_Shell_ExitSystem("cannot find Movie_GetCurrentFrame");
    }

    Movie_GetSoundChannels = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Movie_GetSoundChannels");
    if (!Movie_GetSoundChannels) {
        S_Shell_ExitSystem("cannot find Movie_GetSoundChannels");
    }

    Movie_GetSoundPrecision = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Movie_GetSoundPrecision");
    if (!Movie_GetSoundPrecision) {
        S_Shell_ExitSystem("cannot find Movie_GetSoundPrecision");
    }

    Movie_GetSoundRate = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Movie_GetSoundRate");
    if (!Movie_GetSoundRate) {
        S_Shell_ExitSystem("cannot find Movie_GetSoundRate");
    }

    Movie_GetTotalFrames = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Movie_GetTotalFrames");
    if (!Movie_GetTotalFrames) {
        S_Shell_ExitSystem("cannot find Movie_GetTotalFrames");
    }

    Movie_GetXSize =
        (int32_t(*)(void *))GetProcAddress(m_PlayerModule, "Movie_GetXSize");
    if (!Movie_GetXSize) {
        S_Shell_ExitSystem("cannot find Movie_GetXSize");
    }

    Movie_GetYSize =
        (int32_t(*)(void *))GetProcAddress(m_PlayerModule, "Movie_GetYSize");
    if (!Movie_GetYSize) {
        S_Shell_ExitSystem("cannot find Movie_GetYSize");
    }

    Player_GetDSErrorCode =
        (int32_t(*)())GetProcAddress(m_PlayerModule, "Player_GetDSErrorCode");
    if (!Player_GetDSErrorCode) {
        S_Shell_ExitSystem("cannot find Player_GetDSErrorCode");
    }

    Player_InitMovie =
        (int32_t(*)(void *, uint32_t, uint32_t, const char *, uint32_t))
            GetProcAddress(m_PlayerModule, "Player_InitMovie");
    if (!Player_InitMovie) {
        S_Shell_ExitSystem("cannot find Player_InitMovie");
    }

    Player_InitMoviePlayback = (int32_t(*)(HWND, void *, void *))GetProcAddress(
        m_PlayerModule, "Player_InitMoviePlayback");
    if (!Player_InitMoviePlayback) {
        S_Shell_ExitSystem("cannot find Player_InitMoviePlayback");
    }

    Player_InitPlaybackMode =
        (int32_t(*)(void *, void *, uint32_t, uint32_t))GetProcAddress(
            m_PlayerModule, "Player_InitPlaybackMode");
    if (!Player_InitPlaybackMode) {
        S_Shell_ExitSystem("cannot find Player_InitPlaybackMode");
    }

    Player_InitSound = (int32_t(*)(
        void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
        int32_t))GetProcAddress(m_PlayerModule, "Player_InitSound");
    if (!Player_InitSound) {
        S_Shell_ExitSystem("cannot find Player_InitSound");
    }

    Player_InitSoundSystem = (int32_t(*)(HWND))GetProcAddress(
        m_PlayerModule, "Player_InitSoundSystem");
    if (!Player_InitSoundSystem) {
        S_Shell_ExitSystem("cannot find Player_InitSoundSystem");
    }

    Player_InitVideo = (int32_t(*)(
        void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
        int32_t, int32_t, int32_t, int32_t,
        int32_t))GetProcAddress(m_PlayerModule, "Player_InitVideo");
    if (!Player_InitVideo) {
        S_Shell_ExitSystem("cannot find Player_InitVideo");
    }

    Player_MapVideo = (int32_t(*)(void *, int32_t))GetProcAddress(
        m_PlayerModule, "Player_MapVideo");
    if (!Player_MapVideo) {
        S_Shell_ExitSystem("cannot find Player_MapVideo");
    }

    Player_PassInDirectDrawObject = (int32_t(*)(LPDIRECTDRAW))GetProcAddress(
        m_PlayerModule, "Player_PassInDirectDrawObject");
    if (!Player_PassInDirectDrawObject) {
        S_Shell_ExitSystem("cannot find Player_PassInDirectDrawObject");
    }

    Player_PlayFrame = (int32_t(*)(
        void *, void *, void *, uint32_t, void *, uint32_t, uint32_t,
        uint32_t))GetProcAddress(m_PlayerModule, "Player_PlayFrame");
    if (!Player_PlayFrame) {
        S_Shell_ExitSystem("cannot find Player_PlayFrame");
    }

    Player_ReturnPlaybackMode = (int32_t(*)())GetProcAddress(
        m_PlayerModule, "Player_ReturnPlaybackMode");
    if (!Player_ReturnPlaybackMode) {
        S_Shell_ExitSystem("cannot find Player_ReturnPlaybackMode");
    }

    Player_ShutDownMovie = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Player_ShutDownMovie");
    if (!Player_ShutDownMovie) {
        S_Shell_ExitSystem("cannot find Player_ShutDownMovie");
    }

    Player_ShutDownSound = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Player_ShutDownSound");
    if (!Player_ShutDownSound) {
        S_Shell_ExitSystem("cannot find Player_ShutDownSound");
    }

    Player_ShutDownVideo = (int32_t(*)(void *))GetProcAddress(
        m_PlayerModule, "Player_ShutDownVideo");
    if (!Player_ShutDownVideo) {
        S_Shell_ExitSystem("cannot find Player_ShutDownVideo");
    }

    Player_StartTimer =
        (int32_t(*)(void *))GetProcAddress(m_PlayerModule, "Player_StartTimer");

    if (!Player_StartTimer) {
        S_Shell_ExitSystem("cannot find Player_StartTimer");
    }
    if (Player_PassInDirectDrawObject(DDraw)) {
        S_Shell_ExitSystem("ERROR: Cannot initialise FMV player videosystem");
    }
    if (Player_InitSoundSystem(TombHWND)) {
        Player_GetDSErrorCode();
        S_Shell_ExitSystem("ERROR: Cannot prepare FMV player soundsystem");
    }
}

void S_FMV_Play(const char *file_path)
{
    if (T1MConfig.disable_fmv) {
        return;
    }

    char *full_path = NULL;
    File_GetFullPath(file_path, &full_path);

    GameBuf_Shutdown();

    Screen_SetResolution(2);
    HWR_PrepareFMV();

    void *movie_context = NULL;
    void *fmv_context = NULL;
    void *sound_context = NULL;

    HWR_FMVInit();

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

    if (Player_InitPlaybackMode(TombHWND, fmv_context, 1, 0)) { //
        LOG_ERROR("cannot init playback mode");
        goto cleanup;
    }

    if (SoundIsActive) {
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
