#ifndef T1M_GLOBAL_LIB_H
#define T1M_GLOBAL_LIB_H

#include <ddraw.h>
#include <stdint.h>
#include <windows.h>

void Lib_Init();

extern int32_t (*Movie_GetCurrentFrame)(void *);
extern int32_t (*Movie_GetSoundChannels)(void *);
extern int32_t (*Movie_GetSoundPrecision)(void *);
extern int32_t (*Movie_GetSoundRate)(void *);
extern int32_t (*Movie_GetTotalFrames)(void *);
extern int32_t (*Movie_GetXSize)(void *);
extern int32_t (*Movie_GetYSize)(void *);
extern int32_t (*Player_GetDSErrorCode)();
extern int32_t (*Player_InitMovie)(
    void *, uint32_t, uint32_t, const char *, uint32_t);
extern int32_t (*Player_InitMoviePlayback)(HWND, void *, void *);
extern int32_t (*Player_InitPlaybackMode)(void *, void *, uint32_t, uint32_t);
extern int32_t (*Player_InitSound)(
    void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
    int32_t);
extern int32_t (*Player_InitSoundSystem)(HWND);
extern int32_t (*Player_InitVideo)(
    void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
    int32_t, int32_t, int32_t, int32_t, int32_t);
extern int32_t (*Player_MapVideo)(void *, int32_t);
extern int32_t (*Player_PassInDirectDrawObject)(LPDIRECTDRAW);
extern int32_t (*Player_PlayFrame)(
    void *, void *, void *, uint32_t, void *, uint32_t, uint32_t, uint32_t);
extern int32_t (*Player_ReturnPlaybackMode)();
extern int32_t (*Player_ShutDownMovie)(void *);
extern int32_t (*Player_ShutDownSound)(void *);
extern int32_t (*Player_ShutDownVideo)(void *);
extern int32_t (*Player_StartTimer)(void *);

#endif
