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

#define InitialiseHardware      ((void          (*)())0x00408005)
#define ShutdownHardware        ((void          (*)())0x00408323)
#define DownloadTexturesToHardware  ((void      (*)(int16_t level_num))0x004084DE)
#define PaletteSetHardware      ((void          (*)())0x004087EA)
#define S_DrawSpriteRel         ((void          (*)(int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade))0x00435B70)
#define HardwarePrepareFMV      ((void          (*)())0x0040834C)
#define HardwareFMVDone         ((void          (*)())0x00408368)
#define CopyPictureHardware     ((void          (*)())0x00408B85)
#define DownloadPictureHardware ((void          (*)())0x00408C3A)
#define SwitchResolution        ((void          (*)())0x004089F4)
// clang-format on

void SWRInit();

#endif
