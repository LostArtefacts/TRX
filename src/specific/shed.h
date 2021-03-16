#ifndef T1M_SPECIFIC_SHED_H
#define T1M_SPECIFIC_SHED_H

// a place for odd functions that have no place to go yet

#include <stdint.h>

// clang-format off
#define sub_40837F              ((void          (*)())0x40837F)
#define sub_408E41              ((void          (*)())0x00408E41)
#define sub_4380E0              ((void          (*)(int16_t *unk))0x004380E0)
#define sub_43D940              ((void          (*)())0x43D940)
#define _malloc                 ((void*         (*)(size_t n))0x00441310)

#define WinVidSpinMessageLoop   ((int32_t       (*)())0x00437AD0)
#define ShowFatalError          ((void          (*)(const char *message))0x0043D770)
#define S_ExitSystem            ((void          (*)(const char *message))0x0041E260)
#define InitialiseHardware      ((void          (*)())0x00408005)
#define mn_stop_ambient_samples ((void          (*)())0x0042B000)
#define mn_reset_sound_effects  ((void          (*)())0x0042A940)
#define mn_reset_ambient_loudness ((void        (*)())0x0042AFD0)
#define mn_update_sound_effects ((void          (*)())0x0042B080)
#define adjust_master_volume    ((void          (*)(int32_t new_volume))0x0042B410)
#define CheckCheatMode          ((void          (*)())0x00438920)
#define DownloadTexturesToHardware  ((void      (*)(int16_t level_num))0x004084DE)
#define PaletteSetHardware      ((void          (*)())0x004087EA)
#define S_DrawSpriteRel         ((void          (*)(int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade))0x00435B70)
#define HardwarePrepareFMV      ((void          (*)())0x0040834C)
#define HardwareFMVDone         ((void          (*)())0x00408368)
#define CopyPictureHardware     ((void          (*)())0x00408B85)
#define DownloadPictureHardware ((void          (*)())0x00408C3A)
#define SwitchResolution        ((void          (*)())0x004089F4)

#define Movie_GetCurrentFrame           ((int32_t (*)(void*))0x004504AC)
#define Movie_GetSoundChannels          ((int32_t (*)(void*))0x004504D0)
#define Movie_GetSoundPrecision         ((int32_t (*)(void*))0x004504DC)
#define Movie_GetSoundRate              ((int32_t (*)(void*))0x004504D6)
#define Movie_GetTotalFrames            ((int32_t (*)(void*))0x004504B8)
#define Movie_GetXSize                  ((int32_t (*)(void*))0x004504EE)
#define Movie_GetYSize                  ((int32_t (*)(void*))0x0004504F4)
#define Player_GetDSErrorCode           ((int32_t (*)())0x0045047C)
#define Player_InitMovie                ((int32_t (*)(void*, uint32_t, uint32_t, const char*, uint32_t))0x004504FA)
#define Player_InitMoviePlayback        ((int32_t (*)(void*, void*, void*))0x004504C4)
#define Player_InitPlaybackMode         ((int32_t (*)(int32_t, void*, uint32_t, uint32_t))0x004504E2)
#define Player_InitSound                ((int32_t (*)(void*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t))0x004504CA)
#define Player_InitSoundSystem          ((int32_t (*)(int32_t))0x00450482)
#define Player_InitVideo                ((int32_t (*)(void*, void*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t))0x004504E8)
#define Player_MapVideo                 ((int32_t (*)(void*, int32_t))0x004504BE)
#define Player_PassInDirectDrawObject   ((int32_t (*)(void*))0x00450488)
#define Player_PlayFrame                ((int32_t (*)(void*, void*, void*, uint32_t, void*, uint32_t, uint32_t, uint32_t))0x004504A6)
#define Player_ReturnPlaybackMode       ((int32_t (*)())0x004504A0)
#define Player_ShutDownMovie            ((int32_t (*)(void*))0x0045048E)
#define Player_ShutDownSound            ((int32_t (*)(void*))0x0045049A)
#define Player_ShutDownVideo            ((int32_t (*)(void*))0x00450494)
#define Player_StartTimer               ((int32_t (*)(void*))0x004504B2)
// clang-format on

#endif
