#ifndef T1M_SPECIFIC_FRONTEND_H
#define T1M_SPECIFIC_FRONTEND_H

#include <stdint.h>

// clang-format off
#define S_Colour                ((SG_COL    (*)(int32_t red, int32_t green, int32_t blue))0x0041C0F0)
#define S_DrawScreenSprite2d    ((void      (*)(int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v, int32_t sprnum, int16_t shade, uint16_t flags, int page))0x0041C180)
#define S_DrawScreenSprite      ((void      (*)(int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v, int16_t sprnum, int16_t shade, uint16_t flags))0x0041C2D0)
#define S_DrawScreenLine        ((void      (*)(int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, int32_t col, SG_COL* grdptr, uint16_t flags))0x0041C440)
#define S_DrawScreenBox         ((void      (*)(int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, int32_t col, SG_COL* grdptr, uint16_t flags))0x0041C520)
#define S_DrawScreenFBox        ((void      (*)(int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, int32_t col, SG_COL* grdptr, uint16_t flags))0x0041CBB0)
#define S_FinishInventory       ((void      (*)())0x0041CCC0)
#define S_FadeToBlack           ((void      (*)())0x0041CD10)
#define WinPlayFMV              ((int32_t   (*)(int32_t, int32_t))0x0041CDF0)
#define SoundStart              ((void      (*)())0x0041CDA0)
// clang-format on

void S_Wait(int32_t nframes);
int32_t S_PlayFMV(int32_t sequence, int32_t mode);

void T1MInjectSpecificFrontend();

#endif
