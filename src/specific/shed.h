#ifndef T1M_SPECIFIC_SHED_H
#define T1M_SPECIFIC_SHED_H

// a place for odd functions that have no place to go yet

#include <stdint.h>

// clang-format off
#define sub_408E41              ((void          (*)())0x00408E41)
#define sub_4380E0              ((void          (*)(int16_t *unk))0x004380E0)
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
// clang-format on

#endif
