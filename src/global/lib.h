#ifndef T1M_GLOBAL_LIB_H
#define T1M_GLOBAL_LIB_H

#include <ddraw.h>
#include <stdint.h>
#include <windows.h>

// clang-format off
#define Movie_GetCurrentFrame           ((int32_t (*)(void*))0x004504AC)
#define Movie_GetSoundChannels          ((int32_t (*)(void*))0x004504D0)
#define Movie_GetSoundPrecision         ((int32_t (*)(void*))0x004504DC)
#define Movie_GetSoundRate              ((int32_t (*)(void*))0x004504D6)
#define Movie_GetTotalFrames            ((int32_t (*)(void*))0x004504B8)
#define Movie_GetXSize                  ((int32_t (*)(void*))0x004504EE)
#define Movie_GetYSize                  ((int32_t (*)(void*))0x0004504F4)
#define Player_GetDSErrorCode           ((int32_t (*)())0x0045047C)
#define Player_InitMovie                ((int32_t (*)(void*, uint32_t, uint32_t, const char*, uint32_t))0x004504FA)
#define Player_InitMoviePlayback        ((int32_t (*)(HWND, void*, void*))0x004504C4)
#define Player_InitPlaybackMode         ((int32_t (*)(void*, void*, uint32_t, uint32_t))0x004504E2)
#define Player_InitSound                ((int32_t (*)(void*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t))0x004504CA)
#define Player_InitSoundSystem          ((int32_t (*)(HWND))0x00450482)
#define Player_InitVideo                ((int32_t (*)(void*, void*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t))0x004504E8)
#define Player_MapVideo                 ((int32_t (*)(void*, int32_t))0x004504BE)
#define Player_PassInDirectDrawObject   ((int32_t (*)(LPDIRECTDRAW))0x00450488)
#define Player_PlayFrame                ((int32_t (*)(void*, void*, void*, uint32_t, void*, uint32_t, uint32_t, uint32_t))0x004504A6)
#define Player_ReturnPlaybackMode       ((int32_t (*)())0x004504A0)
#define Player_ShutDownMovie            ((int32_t (*)(void*))0x0045048E)
#define Player_ShutDownSound            ((int32_t (*)(void*))0x0045049A)
#define Player_ShutDownVideo            ((int32_t (*)(void*))0x00450494)
#define Player_StartTimer               ((int32_t (*)(void*))0x004504B2)
// clang-format on

#endif
