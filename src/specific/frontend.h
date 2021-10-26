#ifndef T1M_SPECIFIC_FRONTEND_H
#define T1M_SPECIFIC_FRONTEND_H

#include "global/types.h"

#include <stdint.h>

// clang-format off
// clang-format on

void FMVInit();
int32_t WinPlayFMV(int32_t sequence, int32_t mode);
int32_t S_PlayFMV(int32_t sequence, int32_t mode);

void S_Wait(int32_t nframes);

SG_COL S_Colour(int32_t red, int32_t green, int32_t blue);
RGB888 S_ColourFromPalette(int8_t idx);

void S_DrawScreenFlatQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 color);
void S_DrawScreenGradientQuad(
    int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 tl, RGB888 tr,
    RGB888 bl, RGB888 br);

void S_DrawScreenLine(int32_t sx, int32_t sy, int32_t w, int32_t h, RGB888 col);
void S_DrawScreenBox(int32_t sx, int32_t sy, int32_t w, int32_t h);
void S_DrawScreenFBox(int32_t sx, int32_t sy, int32_t w, int32_t h);
void S_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int16_t sprnum, int16_t shade, uint16_t flags);
void S_DrawScreenSprite2d(
    int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v,
    int32_t sprnum, int16_t shade, uint16_t flags, int page);

void S_FadeToBlack();

void S_FinishInventory();

void T1MInjectSpecificFrontend();

#endif
