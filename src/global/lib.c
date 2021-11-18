#include "global/lib.h"

#include "specific/s_shell.h"

static struct {
    HMODULE player_module;
} S = { 0 };

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

void Lib_Init()
{
    S.player_module = LoadLibraryA("winplay");
    if (!S.player_module) {
        S_Shell_ExitSystem("cannot find winplay.dll");
    }

    Movie_GetCurrentFrame = (int32_t(*)(void *))GetProcAddress(
        S.player_module, "Movie_GetCurrentFrame");
    if (!Movie_GetCurrentFrame) {
        S_Shell_ExitSystem("cannot find Movie_GetCurrentFrame");
    }

    Movie_GetSoundChannels = (int32_t(*)(void *))GetProcAddress(
        S.player_module, "Movie_GetSoundChannels");
    if (!Movie_GetSoundChannels) {
        S_Shell_ExitSystem("cannot find Movie_GetSoundChannels");
    }

    Movie_GetSoundPrecision = (int32_t(*)(void *))GetProcAddress(
        S.player_module, "Movie_GetSoundPrecision");
    if (!Movie_GetSoundPrecision) {
        S_Shell_ExitSystem("cannot find Movie_GetSoundPrecision");
    }

    Movie_GetSoundRate = (int32_t(*)(void *))GetProcAddress(
        S.player_module, "Movie_GetSoundRate");
    if (!Movie_GetSoundRate) {
        S_Shell_ExitSystem("cannot find Movie_GetSoundRate");
    }

    Movie_GetTotalFrames = (int32_t(*)(void *))GetProcAddress(
        S.player_module, "Movie_GetTotalFrames");
    if (!Movie_GetTotalFrames) {
        S_Shell_ExitSystem("cannot find Movie_GetTotalFrames");
    }

    Movie_GetXSize =
        (int32_t(*)(void *))GetProcAddress(S.player_module, "Movie_GetXSize");
    if (!Movie_GetXSize) {
        S_Shell_ExitSystem("cannot find Movie_GetXSize");
    }

    Movie_GetYSize =
        (int32_t(*)(void *))GetProcAddress(S.player_module, "Movie_GetYSize");
    if (!Movie_GetYSize) {
        S_Shell_ExitSystem("cannot find Movie_GetYSize");
    }

    Player_GetDSErrorCode =
        (int32_t(*)())GetProcAddress(S.player_module, "Player_GetDSErrorCode");
    if (!Player_GetDSErrorCode) {
        S_Shell_ExitSystem("cannot find Player_GetDSErrorCode");
    }

    Player_InitMovie =
        (int32_t(*)(void *, uint32_t, uint32_t, const char *, uint32_t))
            GetProcAddress(S.player_module, "Player_InitMovie");
    if (!Player_InitMovie) {
        S_Shell_ExitSystem("cannot find Player_InitMovie");
    }

    Player_InitMoviePlayback = (int32_t(*)(HWND, void *, void *))GetProcAddress(
        S.player_module, "Player_InitMoviePlayback");
    if (!Player_InitMoviePlayback) {
        S_Shell_ExitSystem("cannot find Player_InitMoviePlayback");
    }

    Player_InitPlaybackMode =
        (int32_t(*)(void *, void *, uint32_t, uint32_t))GetProcAddress(
            S.player_module, "Player_InitPlaybackMode");
    if (!Player_InitPlaybackMode) {
        S_Shell_ExitSystem("cannot find Player_InitPlaybackMode");
    }

    Player_InitSound = (int32_t(*)(
        void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
        int32_t))GetProcAddress(S.player_module, "Player_InitSound");
    if (!Player_InitSound) {
        S_Shell_ExitSystem("cannot find Player_InitSound");
    }

    Player_InitSoundSystem = (int32_t(*)(HWND))GetProcAddress(
        S.player_module, "Player_InitSoundSystem");
    if (!Player_InitSoundSystem) {
        S_Shell_ExitSystem("cannot find Player_InitSoundSystem");
    }

    Player_InitVideo = (int32_t(*)(
        void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
        int32_t, int32_t, int32_t, int32_t,
        int32_t))GetProcAddress(S.player_module, "Player_InitVideo");
    if (!Player_InitVideo) {
        S_Shell_ExitSystem("cannot find Player_InitVideo");
    }

    Player_MapVideo = (int32_t(*)(void *, int32_t))GetProcAddress(
        S.player_module, "Player_MapVideo");
    if (!Player_MapVideo) {
        S_Shell_ExitSystem("cannot find Player_MapVideo");
    }

    Player_PassInDirectDrawObject = (int32_t(*)(LPDIRECTDRAW))GetProcAddress(
        S.player_module, "Player_PassInDirectDrawObject");
    if (!Player_PassInDirectDrawObject) {
        S_Shell_ExitSystem("cannot find Player_PassInDirectDrawObject");
    }

    Player_PlayFrame = (int32_t(*)(
        void *, void *, void *, uint32_t, void *, uint32_t, uint32_t,
        uint32_t))GetProcAddress(S.player_module, "Player_PlayFrame");
    if (!Player_PlayFrame) {
        S_Shell_ExitSystem("cannot find Player_PlayFrame");
    }

    Player_ReturnPlaybackMode = (int32_t(*)())GetProcAddress(
        S.player_module, "Player_ReturnPlaybackMode");
    if (!Player_ReturnPlaybackMode) {
        S_Shell_ExitSystem("cannot find Player_ReturnPlaybackMode");
    }

    Player_ShutDownMovie = (int32_t(*)(void *))GetProcAddress(
        S.player_module, "Player_ShutDownMovie");
    if (!Player_ShutDownMovie) {
        S_Shell_ExitSystem("cannot find Player_ShutDownMovie");
    }

    Player_ShutDownSound = (int32_t(*)(void *))GetProcAddress(
        S.player_module, "Player_ShutDownSound");
    if (!Player_ShutDownSound) {
        S_Shell_ExitSystem("cannot find Player_ShutDownSound");
    }

    Player_ShutDownVideo = (int32_t(*)(void *))GetProcAddress(
        S.player_module, "Player_ShutDownVideo");
    if (!Player_ShutDownVideo) {
        S_Shell_ExitSystem("cannot find Player_ShutDownVideo");
    }

    Player_StartTimer = (int32_t(*)(void *))GetProcAddress(
        S.player_module, "Player_StartTimer");
    if (!Player_StartTimer) {
        S_Shell_ExitSystem("cannot find Player_StartTimer");
    }
}
